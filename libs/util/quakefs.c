/*
	quakefs.c

	virtual filesystem functions

	Copyright (C) 1996-1997  Id Software, Inc.

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		59 Temple Place - Suite 330
		Boston, MA  02111-1307, USA

*/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

static __attribute__ ((unused)) const char rcsid[] = 
	"$Id$";

#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_IO_H
# include <io.h>
#endif
#ifdef HAVE_PWD_H
# include <pwd.h>
#endif

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>

#ifdef HAVE_FNMATCH_H
# define model_t sunmodel_t
# include <fnmatch.h>
# undef model_t
#else
# ifdef WIN32
# include "fnmatch.h"
# endif
#endif

#ifdef HAVE_IO_H
# include <io.h>
#endif

#ifdef _MSC_VER
# define _POSIX_
#endif

#include <limits.h>

#include "QF/cmd.h"
#include "QF/cvar.h"
#include "QF/dstring.h"
#include "QF/hash.h"
#include "QF/pak.h"
#include "QF/pakfile.h"
#include "QF/qargs.h"
#include "QF/qendian.h"
#include "QF/qfplist.h"
#include "QF/qtypes.h"
#include "QF/quakefs.h"
#include "QF/sys.h"
#include "QF/va.h"
#include "QF/zone.h"

#include "compat.h"

#ifndef HAVE_FNMATCH_PROTO
int fnmatch (const char *__pattern, const char *__string, int __flags);
#endif

/*
	All of Quake's data access is through a hierchical file system, but the
	contents of the file system can be transparently merged from several
	sources.

	The "user directory" is the path to the directory holding the quake.exe
	and all game directories.  This can be overridden with the "fs_sharepath"
	and "fs_userpath" cvars to allow code debugging in a different directory.
	The base directory is only used during filesystem initialization.

	The "game directory" is the first tree on the search path and directory
	that all generated files (savegames, screenshots, demos, config files)
	will be saved to.  This can be overridden with the "-game" command line
	parameter.  The game directory can never be changed while quake is
	executing.  This is a precacution against having a malicious server
	instruct clients to write files over areas they shouldn't.

	The "cache directory" is only used during development to save network
	bandwidth, especially over ISDN / T1 lines.  If there is a cache directory
	specified, when a file is found by the normal search path, it will be
	mirrored into the cache directory, then opened there.
*/

// QUAKE FILESYSTEM

cvar_t     *fs_userpath;
cvar_t     *fs_sharepath;
cvar_t     *fs_dirconf;

const char *qfs_userpath;

int         qfs_filesize;

searchpath_t *qfs_searchpaths;

//QFS

typedef struct qfs_var_s {
	char       *var;
	char       *val;
} qfs_var_t;

static void QFS_AddGameDirectory (const char *dir);

gamedir_t  *qfs_gamedir;
static plitem_t *qfs_gd_plist;
static const char *qfs_game = "";
static const char *qfs_default_dirconf =
	"{"
	"   QF = {"
	"       Path = \"QF\";"
	"   };"
	"	Quake = {"
	"		Inherit = QF;"
	"		Path = \"id1\";"
	"		GameCode = \"progs.dat\";"
	"	};"
	"	QuakeWorld = {"
	"		Inherit = (Quake);"
	"		Path = \"qw\";"
	"		SkinPath = \"${path}/skins\";"
	"		GameCode = \"qwprogs.dat\";"
	"	};"
	"	\"qw:qw\" = {"
	"		Inherit = (QuakeWorld);"
	"	};"
	"	\"qw:*\" = {"
	"		Inherit = (QuakeWorld);"
	"		Path = \"$gamedir\";"
	"	};"
	"	\"nq:*\" = {"
	"		Inherit = (Quake);"
	"		Path = \"$gamedir\";"
	"	};"
	"}";

static const char *
qfs_var_get_key (void *_v, void *unused)
{
	return ((qfs_var_t *)_v)->var;
}

static void
qfs_var_free (void *_v, void *unused)
{
	qfs_var_t  *v = (qfs_var_t *) _v;
	free (v->var);
	free (v->val);
	free (v);
}

static hashtab_t *
qfs_new_vars (void)
{
	return Hash_NewTable (61, qfs_var_get_key, qfs_var_free, 0);
}

static void
qfs_set_var (hashtab_t *vars, const char *var, const char *val)
{
	qfs_var_t  *v = Hash_Find (vars, var);

	if (!v) {
		v = malloc (sizeof (qfs_var_t));
		v->var = strdup (var);
		v->val = 0;
		Hash_Add (vars, v);
	}
	if (v->val)
		free (v->val);
	v->val = strdup (val);
}

static inline int
qfs_isident (byte c)
{
	return ((c >= 'a' && c <='z') || (c >= 'A' && c <='Z')
			|| (c >= '0' && c <= '9') || c == '_');
}

static char *
qfs_var_subst (const char *string, hashtab_t *vars)
{
	dstring_t  *new = dstring_newstr ();
	const char *s = string;
	const char *e = s;
	const char *var;
	char       *t;
	qfs_var_t  *sub;

	while (1) {
		while (*e && *e != '$')
			e++;
		dstring_appendsubstr (new, s, (e - s));
		if (!*e++)
			break;
		if (*e == '$') {
			dstring_appendstr (new, "$");
			s = ++e;
		} else if (*e == '{') {
			s = e;
			while (*e && *e != '}')
				e++;
			if (!*e) {
				dstring_appendsubstr (new, s, (e - s));
				break;
			}
			var = va ("%.*s", (int) (e - s) - 1, s + 1);
			sub = Hash_Find (vars, var);
			if (sub)
				dstring_appendstr (new, sub->val);
			else
				dstring_appendsubstr (new, s - 1, (e - s) + 2);
			s = ++e;
		} else if (qfs_isident (*e)) {
			s = e;
			while (qfs_isident (*e))
				e++;
			var = va ("%.*s", (int) (e - s), s);
			sub = Hash_Find (vars, var);
			if (sub)
				dstring_appendstr (new, sub->val);
			else
				dstring_appendsubstr (new, s - 1, (e - s) + 1);
			s = e;
		} else {
			dstring_appendstr (new, "$");
			s = e;
		}
	}
	t = new->str;
	free (new);
	return t;
}

static void
qfs_get_gd_params (plitem_t *gdpl, gamedir_t *gamedir, dstring_t *path,
				   hashtab_t *vars)
{
	plitem_t   *p;

	if ((p = PL_ObjectForKey (gdpl, "Path")) && *(char *) p->data) {
		char       *str = qfs_var_subst (p->data, vars);
		char       *e = strchr (str, '"');

		if (!e)
			e = str + strlen (str);
		qfs_set_var (vars, "path", va ("%.*s", (int) (e - str), str));
		if (path->str[0])
			dstring_appendstr (path, ":");
		dstring_appendstr (path, str);
		free (str);
	}
	if (!gamedir->gamecode && (p = PL_ObjectForKey (gdpl, "GameCode")))
		gamedir->gamecode = qfs_var_subst (p->data, vars);
	if (!gamedir->dir.skins && (p = PL_ObjectForKey (gdpl, "SkinPath")))
		gamedir->dir.skins = qfs_var_subst (p->data, vars);
	if (!gamedir->dir.progs && (p = PL_ObjectForKey (gdpl, "ProgPath")))
		gamedir->dir.progs = qfs_var_subst (p->data, vars);
	if (!gamedir->dir.sound && (p = PL_ObjectForKey (gdpl, "SoundPath")))
		gamedir->dir.sound = qfs_var_subst (p->data, vars);
	if (!gamedir->dir.maps && (p = PL_ObjectForKey (gdpl, "MapPath")))
		gamedir->dir.maps = qfs_var_subst (p->data, vars);
}

static void
qfs_inherit (plitem_t *plist, plitem_t *gdpl, gamedir_t *gamedir,
			 dstring_t *path, hashtab_t *dirs, hashtab_t *vars)
{
	plitem_t   *base;

	if (!(base = PL_ObjectForKey (gdpl, "Inherit")))
		return;
	if (base->type == QFString) {
		if (Hash_Find (dirs, base->data))
			return;
		gdpl = PL_ObjectForKey (plist, base->data);
		if (!gdpl) {
			Sys_Printf ("base `%s' not found\n", (char *)base->data);
			return;
		}
		qfs_set_var (vars, "gamedir", base->data);
		Hash_Add (dirs, base->data);
		qfs_get_gd_params (gdpl, gamedir, path, vars);
		qfs_inherit (plist, gdpl, gamedir, path, dirs, vars);
	} else if (base->type == QFArray) {
		int         i;
		plarray_t  *a = base->data;
		for (i = 0; i < a->numvals; i++) {
			base = a->values[i];
			if (Hash_Find (dirs, base->data))
				continue;
			gdpl = PL_ObjectForKey (plist, base->data);
			if (!gdpl) {
				Sys_Printf ("base `%s' not found\n", (char *)base->data);
				continue;
			}
			qfs_set_var (vars, "gamedir", base->data);
			Hash_Add (dirs, base->data);
			qfs_get_gd_params (gdpl, gamedir, path, vars);
			qfs_inherit (plist, gdpl, gamedir, path, dirs, vars);
		}
	}
}

static int
qfs_compare (const void *a, const void *b)
{
	return strcmp ((*(dictkey_t **)a)->key, (*(dictkey_t **)b)->key);
}

static const char *
qfs_dir_get_key (void *_k, void *unused)
{
	return _k;
}

static plitem_t *
qfs_find_gamedir (const char *name, hashtab_t *dirs)
{
	plitem_t   *gdpl = PL_ObjectForKey (qfs_gd_plist, name);

	if (!gdpl) {
		dictkey_t **list = (dictkey_t **) Hash_GetList (qfs_gd_plist->data);
		dictkey_t **l;
		for (l = list; *l; l++)
			;
		qsort (list, l - list, sizeof (char *), qfs_compare);
		while (l-- != list) {
			if (!fnmatch ((*l)->key, name, 0)) {
				gdpl = (*l)->value;
				Hash_Add (dirs, (*l)->key);
				break;
			}
		}
		free (list);
	}
	return gdpl;
}

static void
qfs_process_path (const char *path, const char *gamedir)
{
	const char *e = path + strlen (path);
	const char *s = e;
	dstring_t  *dir = dstring_new ();

	while (s >= path) {
		while (s != path && s[-1] !=':')
			s--;
		if (s != e) {
			dsprintf (dir, "%.*s", (int) (e - s), s);
			QFS_AddGameDirectory (dir->str);
		}
		e = --s;
	}
	dstring_delete (dir);
}

static void
qfs_build_gamedir (const char **list)
{
	int         j;
	gamedir_t  *gamedir;
	plitem_t   *gdpl;
	dstring_t  *path;
	hashtab_t  *dirs = Hash_NewTable (31, qfs_dir_get_key, 0, 0);
	hashtab_t  *vars = qfs_new_vars ();
	const char *dir = 0;

	qfs_set_var (vars, "game", qfs_game);

	if (qfs_gamedir) {
		if (qfs_gamedir->name)
			free ((char *)qfs_gamedir->name);
		if (qfs_gamedir->gamedir)
			free ((char *)qfs_gamedir->gamedir);
		if (qfs_gamedir->path)
			free ((char *)qfs_gamedir->path);
		if (qfs_gamedir->gamecode)
			free ((char *)qfs_gamedir->gamecode);
		if (qfs_gamedir->dir.def)
			free ((char *)qfs_gamedir->dir.def);
		if (qfs_gamedir->dir.skins)
			free ((char *)qfs_gamedir->dir.skins);
		if (qfs_gamedir->dir.progs)
			free ((char *)qfs_gamedir->dir.progs);
		if (qfs_gamedir->dir.sound)
			free ((char *)qfs_gamedir->dir.sound);
		if (qfs_gamedir->dir.maps)
			free ((char *)qfs_gamedir->dir.maps);
		free (qfs_gamedir);
	}

	while (qfs_searchpaths) {
		searchpath_t *next;

		if (qfs_searchpaths->pack) {
			Qclose (qfs_searchpaths->pack->handle);
			free (qfs_searchpaths->pack->files);
			free (qfs_searchpaths->pack);
		}
		next = qfs_searchpaths->next;
		free (qfs_searchpaths);
		qfs_searchpaths = next;
	}

	for (j = 0; list[j]; j++)
		;
	gamedir = calloc (1, sizeof (gamedir_t));
	path = dstring_newstr ();
	while (j--) {
		const char *name = va ("%s:%s", qfs_game, dir = list[j]);
		if (Hash_Find (dirs, name))
			continue;
		gdpl = qfs_find_gamedir (name, dirs);
		if (!gdpl) {
			Sys_Printf ("gamedir `%s' not found\n", name);
			continue;
		}
		Hash_Add (dirs, (void *) name);
		if (!j) {
			gamedir->name = strdup (name);
			gamedir->gamedir = strdup (list[j]);
		}
		qfs_set_var (vars, "gamedir", dir);
		qfs_get_gd_params (gdpl, gamedir, path, vars);
		qfs_inherit (qfs_gd_plist, gdpl, gamedir, path, dirs, vars);
	}
	gamedir->path = path->str;

	for (dir = gamedir->path; *dir && *dir != ':'; dir++)
		;
	gamedir->dir.def = nva ("%.*s", (int) (dir - gamedir->path),
							gamedir->path);
	if (!gamedir->dir.skins)
		gamedir->dir.skins = nva ("%s/skins", gamedir->dir.def);
	if (!gamedir->dir.progs)
		gamedir->dir.progs = nva ("%s/progs", gamedir->dir.def);
	if (!gamedir->dir.sound)
		gamedir->dir.sound = nva ("%s/sound", gamedir->dir.def);
	if (!gamedir->dir.maps)
		gamedir->dir.maps = nva ("%s/maps", gamedir->dir.def);

	qfs_gamedir = gamedir;
	Sys_DPrintf ("%s\n", qfs_gamedir->name);
	Sys_DPrintf ("    gamedir : %s\n", qfs_gamedir->gamedir);
	Sys_DPrintf ("    path    : %s\n", qfs_gamedir->path);
	Sys_DPrintf ("    gamecode: %s\n", qfs_gamedir->gamecode);
	Sys_DPrintf ("    def     : %s\n", qfs_gamedir->dir.def);
	Sys_DPrintf ("    skins   : %s\n", qfs_gamedir->dir.skins);
	Sys_DPrintf ("    progs   : %s\n", qfs_gamedir->dir.progs);
	Sys_DPrintf ("    sound   : %s\n", qfs_gamedir->dir.sound);
	Sys_DPrintf ("    maps    : %s\n", qfs_gamedir->dir.maps);
	qfs_process_path (qfs_gamedir->path, dir);
	free (path);
	Hash_DelTable (dirs);
	Hash_DelTable (vars);
}

static void
qfs_load_config (void)
{
	QFile      *f;
	int         len;
	char       *buf;

	if (!(f = Qopen (fs_dirconf->string, "rt"))) {
		Sys_DPrintf ("Could not load `%s', using builtin defaults\n",
					fs_dirconf->string);
		goto no_config;
	}
	len = Qfilesize (f);
	buf = malloc (len + 3); // +3 for { } and \0

	Qread (f, buf + 1, len);
	Qclose (f);

	// convert the config file to a plist dictionary
	buf[0] = '{';
	buf[len + 1] = '}';
	buf[len + 2] = 0;
	qfs_gd_plist = PL_GetPropertyList (buf);
	free (buf);
	if (qfs_gd_plist && qfs_gd_plist->type == QFDictionary)
		return;		// done
	Sys_Printf ("not a dictionary\n");
no_config:
	qfs_gd_plist = PL_GetPropertyList (qfs_default_dirconf);
}

char *
QFS_FileBase (const char *in)
{
	const char *slash, *dot, *s;
	char       *out;

	slash = in;
	dot = NULL;
	s = in;
	while (*s) {
		if (*s == '/')
			slash = s + 1;
		if (*s == '.')
			dot = s;
		s++;
	}
	if (dot == NULL)
		dot = s;
	if (dot - slash < 2)
		return strdup ("?model?");
	out = malloc (dot - slash + 1);
	strncpy (out, slash, dot - slash);
	out [dot - slash] = 0;
	return out;
}


static void
QFS_Path_f (void)
{
	searchpath_t *s;

	Sys_Printf ("Current search path:\n");
	for (s = qfs_searchpaths; s; s = s->next) {
		if (s->pack)
			Sys_Printf ("%s (%i files)\n", s->pack->filename,
						s->pack->numfiles);
		else
			Sys_Printf ("%s\n", s->filename);
	}
}

/*
	QFS_WriteFile

	The filename will be prefixed by the current game directory
*/
void
QFS_WriteFile (const char *filename, void *data, int len)
{
	QFile      *f;

	f = QFS_WOpen (filename, 0);
	if (!f) {
		Sys_Error ("Error opening %s", filename);
	}

	Qwrite (f, data, len);
	Qclose (f);
}

/*
	QFS_WriteBuffers

	The filename will be prefixed by the current game directory
*/
void
QFS_WriteBuffers (const char *filename, int count, ...)
{
	va_list     args;
	QFile      *f;

	va_start (args, count);

	f = QFS_WOpen (filename, 0);
	if (!f) {
		Sys_Error ("Error opening %s", filename);
	}

	Sys_Printf ("QFS_WriteBuffers: %s\n", filename);
	while (count--) {
		void       *data = va_arg (args, void *);
		int         len = va_arg (args, int);

		Qwrite (f, data, len);
	}
	Qclose (f);
	va_end (args);
}

/*
	QFS_CreatePath

	Only used for CopyFile and download
*/
void
QFS_CreatePath (const char *path)
{
	char       *ofs;
	char       *e_path = alloca (strlen (path) + 1);

	strcpy (e_path, path);
	for (ofs = e_path + 1; *ofs; ofs++) {
		if (*ofs == '/') {				// create the directory
			*ofs = 0;
			Sys_mkdir (e_path);
			*ofs = '/';
		}
	}
}

static QFile *
QFS_OpenRead (const char *path, int offs, int len, int zip)
{
	QFile      *file;

	if (offs < 0 || len < 0)
		file = Qopen (path, zip ? "rbz" : "rb");
	else
		file = Qsubopen (path, offs, len, zip);

	if (!file) {
		Sys_Error ("Couldn't open %s", path);
		return 0;
	}

	qfs_filesize = Qfilesize (file);

	return file;
}

char *
QFS_CompressPath (const char *pth)
{
	char       *p, *d;
	char       *path= malloc (strlen (pth) + 1);

	for (d = path; *pth; d++, pth++) {
		if (*pth == '\\')
			*d = '/';
		else
			*d = *pth;
	}
	*d = 0;

	p = path;
	while (*p) {
		if (p[0] == '.') {
			if (p[1] == '.') {
				if (p[2] == '/' || p[2] == 0) {
					d = p;
					if (d > path)
						d--;
					while (d > path && d[-1] != '/')
						d--;
					if (d == path
						&& d[0] == '.' && d[1] == '.'
						&& (d[2] == '/' || d[2] == '0')) {
						p += 2 + (p[2] == '/');
						continue;
					}
					strcpy (d, p + 2 + (p[2] == '/'));
					p = d;
					continue;
				}
			} else if (p[1] == '/') {
				strcpy (p, p + 2);
				continue;
			} else if (p[1] == 0) {
				p[0] = 0;
			}
		}
		while (*p && *p != '/')
			p++;
		if (*p == '/')
			p++;
	}

	return path;
}

/*
	contains_updir

	Checks if a string contains an updir ('..'), either at the ends or
	surrounded by slashes ('/').  Doesn't check for leading slash.
*/
static int
contains_updir (const char *filename)
{
	int i;

	if (filename[0] == 0 || filename [1] == 0)
                return 0;

	for (i = 0; filename[i+1]; i++) {
		if (!(i == 0 || filename[i-1] == '/')           // beginning of string
														// or first slash
		    || filename[i] != '.'                       // first dot
		    || filename[i+1] != '.')                    // second dot
			continue;

		if (filename[i+2] == 0 || filename[i+2] == '/')
			// end of string or second slash
			return 1;
	}
	return 0;
}

int file_from_pak; // global indicating file came from pack file ZOID

/*
	QFS_FOpenFile

	Finds the file in the search path.
	Sets qfs_filesize and one of handle or file
*/

static int
open_file (searchpath_t *search, const char *filename, QFile **gzfile,
		   dstring_t *foundname, int zip)
{
	char       *netpath;

	file_from_pak = 0;

	// is the element a pak file?
	if (search->pack) {
		dpackfile_t *packfile;

		packfile = pack_find_file (search->pack, filename);
		if (packfile) {
			Sys_DPrintf ("PackFile: %s : %s\n", search->pack->filename,
						 packfile->name);
			// open a new file on the pakfile
			if (foundname) {
				dstring_clearstr (foundname);
				dstring_appendstr (foundname, packfile->name);
			}
			*gzfile = QFS_OpenRead (search->pack->filename, packfile->filepos,
									packfile->filelen, zip);
			file_from_pak = 1;
			return qfs_filesize;
		}
	} else {
		// check a file in the directory tree
		netpath = nva ("%s/%s", search->filename, filename);

		if (foundname) {
			dstring_clearstr (foundname);
			dstring_appendstr (foundname, filename);
		}
		if (Sys_FileTime (netpath) == -1) {
			free (netpath);
			return -1;
		}

		Sys_DPrintf ("FindFile: %s\n", netpath);

		*gzfile = QFS_OpenRead (netpath, -1, -1, zip);
		free (netpath);
		return qfs_filesize;
	}

	return -1;
}

int
_QFS_FOpenFile (const char *filename, QFile **gzfile,
				dstring_t *foundname, int zip)
{
	searchpath_t *search;
	char       *path;
#ifdef HAVE_VORBIS
	char       *oggfilename;
#endif
#ifdef HAVE_ZLIB
	char       *gzfilename;
#endif

	// make sure they're not trying to do weird stuff with our private files
	path = QFS_CompressPath (filename);
	if (contains_updir(path)) {
		Sys_Printf ("FindFile: %s: attempt to escape directory tree!\n", path);
		goto error;
	}

#ifdef HAVE_VORBIS
	if (strequal (".wav", QFS_FileExtension (path))) {
		oggfilename = alloca (strlen (path) + 1);
		QFS_StripExtension (path, oggfilename);
		strncat (oggfilename, ".ogg",
				 sizeof (oggfilename) - strlen (oggfilename) - 1);
	} else {
		oggfilename = 0;
	}
#endif
#ifdef HAVE_ZLIB
	gzfilename = alloca (strlen (path) + 3 + 1);
	sprintf (gzfilename, "%s.gz", path);
#endif

	// search through the path, one element at a time
	for (search = qfs_searchpaths; search; search = search->next) {
#ifdef HAVE_VORBIS
		//NOTE gzipped oggs not supported
		if (oggfilename
			&& open_file (search, oggfilename, gzfile, foundname, false) != -1)
			goto ok;
#endif
#ifdef HAVE_ZLIB
		if (open_file (search, gzfilename, gzfile, foundname, zip) != -1)
			goto ok;
#endif
		if (open_file (search, path, gzfile, foundname, zip) != -1)
			goto ok;
	}

	Sys_DPrintf ("FindFile: can't find %s\n", filename);
error:
	*gzfile = NULL;
	qfs_filesize = -1;
	free (path);
	return -1;
ok:
	free(path);
	return qfs_filesize;
}

int
QFS_FOpenFile (const char *filename, QFile **gzfile)
{
	return _QFS_FOpenFile (filename, gzfile, 0, 1);
}

cache_user_t *loadcache;
byte       *loadbuf;
int         loadsize;

/*
	QFS_LoadFile

	Filename are relative to the quake directory.
	Always appends a 0 byte to the loaded data.
*/
byte *
QFS_LoadFile (const char *path, int usehunk)
{
	QFile      *h;
	byte       *buf = NULL;
	char       *base;
	int         len;

	// look for it in the filesystem or pack files
	len = qfs_filesize = QFS_FOpenFile (path, &h);
	if (!h)
		return NULL;

	// extract the filename base name for hunk tag
	base = QFS_FileBase (path);

	if (usehunk == 1)
		buf = Hunk_AllocName (len + 1, base);
	else if (usehunk == 2)
		buf = Hunk_TempAlloc (len + 1);
	else if (usehunk == 0)
		buf = calloc (1, len + 1);
	else if (usehunk == 3)
		buf = Cache_Alloc (loadcache, len + 1, base);
	else if (usehunk == 4) {
		if (len + 1 > loadsize)
			buf = Hunk_TempAlloc (len + 1);
		else
			buf = loadbuf;
	} else
		Sys_Error ("QFS_LoadFile: bad usehunk");

	if (!buf)
		Sys_Error ("QFS_LoadFile: not enough space for %s", path);

	buf[len] = 0;
	Qread (h, buf, len);
	Qclose (h);

	free (base);

	return buf;
}

byte       *
QFS_LoadHunkFile (const char *path)
{
	return QFS_LoadFile (path, 1);
}

byte       *
QFS_LoadTempFile (const char *path)
{
	return QFS_LoadFile (path, 2);
}

void
QFS_LoadCacheFile (const char *path, struct cache_user_s *cu)
{
	loadcache = cu;
	QFS_LoadFile (path, 3);
}

// uses temp hunk if larger than bufsize
byte       *
QFS_LoadStackFile (const char *path, void *buffer, int bufsize)
{
	byte       *buf;

	loadbuf = (byte *) buffer;
	loadsize = bufsize;
	buf = QFS_LoadFile (path, 4);

	return buf;
}

/*
	QFS_LoadPackFile

	Takes an explicit (not game tree related) path to a pak file.

	Loads the header and directory, adding the files at the beginning
	of the list so they override previous pack files.
*/
static pack_t     *
QFS_LoadPackFile (char *packfile)
{
	pack_t     *pack = pack_open (packfile);

	if (pack)
		Sys_Printf ("Added packfile %s (%i files)\n",
					packfile, pack->numfiles);
	return pack;
}

#define FBLOCK_SIZE	32
#define FNAME_SIZE	MAX_OSPATH

// Note, this is /NOT/ a work-alike strcmp, this groups numbers sanely.
static int
qstrcmp (char **os1, char **os2)
{
	int         in1, in2, n1, n2;
	char       *s1, *s2;

	s1 = *os1;
	s2 = *os2;

	while (1) {
		in1 = in2 = n1 = n2 = 0;

		if ((in1 = isdigit ((byte) *s1)))
			n1 = strtol (s1, &s1, 10);

		if ((in2 = isdigit ((byte) *s2)))
			n2 = strtol (s2, &s2, 10);

		if (in1 && in2) {
			if (n1 != n2)
				return n1 - n2;
		} else {
			if (*s1 != *s2)
				return *s1 - *s2;
			else if (*s1 == '\0')
				return *s1 - *s2;
			s1++;
			s2++;
		}
	}
}

static void
QFS_LoadGameDirectory (const char *dir)
{
	searchpath_t *search;
	pack_t     *pak;
	DIR        *dir_ptr;
	struct dirent *dirent;
	char      **pakfiles = NULL;
	int         i = 0, bufsize = 0, count = 0;

	Sys_DPrintf ("QFS_LoadGameDirectory (\"%s\")\n", dir);

	pakfiles = calloc (1, FBLOCK_SIZE * sizeof (char *));

	bufsize += FBLOCK_SIZE;
	if (!pakfiles)
		goto QFS_LoadGameDirectory_free;

	for (i = 0; i < bufsize; i++) {
		pakfiles[i] = NULL;
	}

	dir_ptr = opendir (dir);
	if (!dir_ptr)
		goto QFS_LoadGameDirectory_free;

	while ((dirent = readdir (dir_ptr))) {
		if (!fnmatch ("*.pak", dirent->d_name, 0)) {
			if (count >= bufsize) {
				bufsize += FBLOCK_SIZE;
				pakfiles = realloc (pakfiles, bufsize * sizeof (char *));

				if (!pakfiles)
					goto QFS_LoadGameDirectory_free;
				for (i = count; i < bufsize; i++)
					pakfiles[i] = NULL;
			}

			pakfiles[count] = malloc (FNAME_SIZE);
			if (!pakfiles[count])
				Sys_Error ("QFS_LoadGameDirectory: MemoryAllocationFailure");
			snprintf (pakfiles[count], FNAME_SIZE - 1, "%s/%s", dir,
					  dirent->d_name);
			pakfiles[count][FNAME_SIZE - 1] = '\0';
			count++;
		}
	}
	closedir (dir_ptr);

	// XXX WARNING!!! This is /NOT/ subtable for strcmp!!!!!
	// This passes 'void **' instead of 'char *' to the cmp function!
	qsort (pakfiles, count, sizeof (char *),
		   (int (*)(const void *, const void *)) qstrcmp);

	for (i = 0; i < count; i++) {
		pak = QFS_LoadPackFile (pakfiles[i]);

		if (!pak) {
			Sys_Error (va ("Bad pakfile %s!!", pakfiles[i]));
		} else {
			search = calloc (1, sizeof (searchpath_t));
			search->pack = pak;
			search->next = qfs_searchpaths;
			qfs_searchpaths = search;
		}
	}

  QFS_LoadGameDirectory_free:
	for (i = 0; i < count; i++)
		free (pakfiles[i]);
	free (pakfiles);
}

/*
	QFS_AddDirectory

	Adds the directory to the head of the path, then loads and adds pak0.pak
	pak1.pak ...
*/
static void
QFS_AddDirectory (const char *dir)
{
	searchpath_t *search;

	// add the directory to the search path
	search = calloc (1, sizeof (searchpath_t));
	strcpy (search->filename, dir);
	search->next = qfs_searchpaths;
	qfs_searchpaths = search;

	// add any pak files in the format pak0.pak pak1.pak, ...
	QFS_LoadGameDirectory (dir);
}

static void
QFS_AddGameDirectory (const char *dir)
{
	if (!*dir)
		return;
	Sys_DPrintf ("QFS_AddGameDirectory (\"%s/%s\")\n",
				 fs_sharepath->string, dir);

	if (strcmp (fs_sharepath->string, fs_userpath->string) != 0)
		QFS_AddDirectory (va ("%s/%s", fs_sharepath->string, dir));
	QFS_AddDirectory (va ("%s/%s", qfs_userpath, dir));
}

/*
	QFS_Gamedir

	Sets the gamedir and path to a different directory.
*/
void
QFS_Gamedir (const char *dir)
{
	const char *list[2] = {dir, 0};

	qfs_build_gamedir (list);
	// flush all data, so it will be forced to reload
	Cache_Flush ();
}

char *
expand_squiggle (const char *path)
{
	char       *home;

#ifndef _WIN32
	struct passwd *pwd_ent;
#endif

	if (strncmp (path, "~/", 2) != 0) {
		return strdup (path);
	}

#ifdef _WIN32
	// LordHavoc: first check HOME to duplicate previous version behavior
	// (also handy if someone wants it elsewhere than their windows directory)
	home = getenv ("HOME");
	if (!home || !home[0])
		home = getenv ("WINDIR");
#else
	if ((pwd_ent = getpwuid (getuid ()))) {
		home = pwd_ent->pw_dir;
	} else
		home = getenv ("HOME");
#endif

	if (home)
		return nva ("%s%s", home, path + 1);	// skip leading ~

	return strdup (path);
}

void
QFS_Init (const char *game)
{
	int         i;

	fs_sharepath = Cvar_Get ("fs_sharepath", FS_SHAREPATH, CVAR_ROM, NULL,
							 "location of shared (read only) game "
							 "directories");
	fs_userpath = Cvar_Get ("fs_userpath", FS_USERPATH, CVAR_ROM, NULL,
							"location of your game directories");
	fs_dirconf = Cvar_Get ("fs_dirconf", "", CVAR_ROM, NULL,
							"full path to gamedir.conf FIXME");

	Cmd_AddCommand ("path", QFS_Path_f, "Show what paths Quake is using");

	qfs_userpath = expand_squiggle (fs_userpath->string);

	qfs_load_config ();

	qfs_game = game;

	if ((i = COM_CheckParm ("-game")) && i < com_argc - 1) {
		char       *gamedirs = NULL;
		const char **list;
		char       *where;
		int         j, count = 1;

		gamedirs = strdup (com_argv[i + 1]);

		for (j = 0; gamedirs[j]; j++)
			if (gamedirs[j] == ',')
				count++;

		list = calloc (count + 1, sizeof (char *));

		j = 0;
		where = strtok (gamedirs, ",");
		while (where) {
			list[j++] = where;
			where = strtok (NULL, ",");
		}
		qfs_build_gamedir (list);
		free (gamedirs);
		free (list);
	} else {
		QFS_Gamedir ("");
	}
}

const char *
QFS_SkipPath (const char *pathname)
{
	const char *last;

	// char after last / on the line
	if ((last = strrchr (pathname, '/')))
		last++;
	else
		last = pathname;

	return last;
}

void
QFS_StripExtension (const char *in, char *out)
{
	while (*in && *in != '.')
		*out++ = *in++;
	*out = 0;
}

const char *
QFS_FileExtension (const char *in)
{
	while (*in && *in != '.')
		in++;
	return in;
}

void
QFS_DefaultExtension (char *path, const char *extension)
{
	char       *src;

	// if path doesn't have a .EXT, append extension
	// (extension should include the .)
	src = path + strlen (path) - 1;

	while (*src != '/' && src != path) {
		if (*src == '.')
			return;						// it has an extension
		src--;
	}

	strncat (path, extension, MAX_OSPATH - strlen (path));
}

int
QFS_NextFilename (dstring_t *filename, const char *prefix, const char *ext)
{
	char       *digits;
	int         i;

	dsprintf (filename, "%s0000%s", prefix, ext);
	digits = filename->str + strlen (prefix);

	for (i = 0; i <= 9999; i++) {
		digits[0] = i / 1000 + '0';
		digits[1] = i / 100 % 10 + '0';
		digits[2] = i / 10 % 10 + '0';
		digits[3] = i % 10 + '0';
		if (Sys_FileTime (va ("%s/%s", qfs_userpath, filename->str)) == -1)
			return 1;					// file doesn't exist
	}
	return 0;
}

QFile *
QFS_Open (const char *path, const char *mode)
{
	dstring_t  *full_path = dstring_new ();
	QFile      *file;

	dsprintf (full_path, "%s/%s", qfs_userpath, path);
	Sys_DPrintf ("QFS_Open: %s %s\n", full_path->str, mode);
	QFS_CreatePath (full_path->str);
	file = Qopen (full_path->str, mode);
	dstring_delete (full_path);
	return file;
}

QFile *
QFS_WOpen (const char *path, int zip)
{
	char        mode[5] = "wb\000\000";

	if (zip) {
		mode[2] = 'z';
		mode[3] = bound (1, zip, 9) + '0';
	}
	return QFS_Open (path, mode);
}

int
QFS_Rename (const char *old, const char *new)
{
	dstring_t  *full_old = dstring_new ();
	dstring_t  *full_new = dstring_new ();
	int         ret;

	dsprintf (full_old, "%s/%s", qfs_userpath, old);
	dsprintf (full_new, "%s/%s", qfs_userpath, new);
	QFS_CreatePath (full_new->str);
	ret = Qrename (full_old->str, full_new->str);
	dstring_delete (full_old);
	dstring_delete (full_new);
	return ret;
}

int
QFS_Remove (const char *path)
{
	dstring_t  *full_path = dstring_new ();
	int         ret;

	dsprintf (full_path, "%s/%s", qfs_userpath, path);
	ret = Qremove (full_path->str);
	dstring_delete (full_path);
	return ret;
}
