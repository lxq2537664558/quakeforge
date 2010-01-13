/*
	screen.c

	master for refresh, status bar, console, chat, notify, etc

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

static __attribute__ ((used)) const char rcsid[] = 
	"$Id$";

#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include "QF/cmd.h"
#include "QF/cvar.h"
#include "QF/draw.h"
#include "QF/image.h"
#include "QF/pcx.h"
#include "QF/render.h"
#include "QF/screen.h"
#include "QF/sys.h"
#include "QF/view.h"

#include "compat.h"
#include "r_cvar.h"
#include "r_local.h"
#include "r_screen.h"
#include "sbar.h"

/*
	background clear
	rendering
	turtle/net/ram icons
	sbar
	centerprint / slow centerprint
	notify lines
	intermission / finale overlay
	loading plaque
	console
	menu

	required background clears
	required update regions

	syncronous draw mode or async
	One off screen buffer, with updates either copied or xblited
	Need to double buffer?

	async draw will require the refresh area to be cleared, because it will be
	xblited, but sync draw can just ignore it.

	sync
	draw

	CenterPrint ()
	SlowPrint ()
	Screen_Update ();
	Sys_Printf ();

	net
	turn off messages option

	the refresh is always rendered, unless the console is full screen

	console is:
		notify lines
		half
		full
*/

// only the refresh window will be updated unless these variables are flagged 
int         scr_copytop;
int         scr_copyeverything;

float       oldfov;
int         oldsbar;

qboolean    scr_initialized;			// ready to draw

qpic_t     *scr_ram;
qpic_t     *scr_net;
qpic_t     *scr_turtle;

int         scr_fullupdate;

int         clearconsole;
int         clearnotify;

viddef_t    vid;						// global video state

vrect_t    *pconupdate;
vrect_t     scr_vrect;

qboolean    scr_skipupdate;

void
R_SetVrect (vrect_t *pvrectin, vrect_t *pvrect, int lineadj)
{
	float       size;
	int         h;

	// intermission is always full screen
	if (r_viewsize >= 100 || r_force_fullscreen /* FIXME: better test */) {
		size = 100.0;
		lineadj = 0;
	} else {
		size = r_viewsize;
	}
	size /= 100.0;

	h = pvrectin->height - lineadj;

	pvrect->width = pvrectin->width * size + 0.5;
	if (pvrect->width < 96) {
		size = 96.0 / pvrectin->width;
		pvrect->width = 96;				// min for icons
	}
	pvrect->width &= ~7;

	pvrect->height = pvrectin->height * size + 0.5;
	if (pvrect->height > h)
		pvrect->height = h;
	pvrect->height &= ~1;

	pvrect->x = (pvrectin->width - pvrect->width) / 2;
	pvrect->y = (h - pvrect->height) / 2;
}

void
SCR_CalcRefdef (void)
{
	vrect_t     vrect;

	// force a background redraw
	scr_fullupdate = 0;
	vid.recalc_refdef = 0;

	// force the status bar to redraw
	Sbar_Changed ();

	// bound field of view
	Cvar_SetValue (scr_fov, bound (1, scr_fov->value, 170));

	vrect.x = 0;
	vrect.y = 0;
	vrect.width = vid.width;
	vrect.height = vid.height;

	R_SetVrect (&vrect, &scr_vrect, r_lineadj);

	r_refdef.vrect = scr_vrect;
	r_refdef.fov_x = scr_fov->value;
	r_refdef.fov_y =
		CalcFov (r_refdef.fov_x, r_refdef.vrect.width, r_refdef.vrect.height);

	// notify the refresh of the change
	R_ViewChanged (vid.aspect);
}

float
CalcFov (float fov_x, float width, float height)
{
	float       a, x;

	if (fov_x < 1 || fov_x > 179)
		Sys_Error ("Bad fov: %f", fov_x);

	x = width / tan (fov_x * (M_PI / 360));

	a = (x == 0) ? 90 : atan (height / x);	// 0 shouldn't happen

	a = a * (360 / M_PI);

	return a;
}

/*
	SCR_SizeUp_f

	Keybinding command
*/
static void
SCR_SizeUp_f (void)
{
	if (scr_viewsize->int_val < 120) {
		Cvar_SetValue (scr_viewsize, scr_viewsize->int_val + 10);
		vid.recalc_refdef = 1;
	}
}

/*
	SCR_SizeDown_f

	Keybinding command
*/
static void
SCR_SizeDown_f (void)
{
	Cvar_SetValue (scr_viewsize, scr_viewsize->int_val - 10);
	vid.recalc_refdef = 1;
}

void
SCR_DrawRam (void)
{
	if (!scr_showram->int_val)
		return;

	if (!r_cache_thrash)
		return;

	Draw_Pic (scr_vrect.x + 32, scr_vrect.y, scr_ram);
}

void
SCR_DrawTurtle (void)
{
	static int  count;

	if (!scr_showturtle->int_val)
		return;

	if (r_frametime < 0.1) {
		count = 0;
		return;
	}

	count++;
	if (count < 3)
		return;

	Draw_Pic (scr_vrect.x, scr_vrect.y, scr_turtle);
}

void
SCR_DrawPause (void)
{
	qpic_t     *pic;

	if (!scr_showpause->int_val)		// turn off for screenshots
		return;

	if (!r_paused)
		return;

	pic = Draw_CachePic ("gfx/pause.lmp", true);
	Draw_Pic ((vid.conwidth - pic->width) / 2,
			  (vid.conheight - 48 - pic->height) / 2, pic);
}

void
SCR_SetUpToDrawConsole (void)
{
	if (clearconsole++ < vid.numpages)
		Sbar_Changed ();
}

/*
	Find closest color in the palette for named color
*/
int
MipColor (int r, int g, int b)
{
	float       bestdist, dist;
	int         r1, g1, b1, i;
	int         best = 0;
	static int  lastbest;
	static int  lr = -1, lg = -1, lb = -1;

	if (r == lr && g == lg && b == lb)
		return lastbest;

	bestdist = 256 * 256 * 3;

	for (i = 0; i < 256; i++) {
		int         j;
		j = i * 3;
		r1 = vid.palette[j] - r;
		g1 = vid.palette[j + 1] - g;
		b1 = vid.palette[j + 2] - b;
		dist = r1 * r1 + g1 * g1 + b1 * b1;
		if (dist < bestdist) {
			bestdist = dist;
			best = i;
		}
	}
	lr = r;
	lg = g;
	lb = b;
	lastbest = best;
	return best;
}

// in draw.c

static void
SCR_DrawCharToSnap (int num, byte *dest, int width)
{
	byte       *source;
	int         col, row, drawline, x;

	row = num >> 4;
	col = num & 15;
	source = draw_chars + (row << 10) + (col << 3);

	drawline = 8;

	while (drawline--) {
		for (x = 0; x < 8; x++)
			if (source[x])
				dest[x] = source[x];
			else
				dest[x] = 98;
		source += 128;
		dest -= width;
	}

}

void
SCR_DrawStringToSnap (const char *s, tex_t *tex, int x, int y)
{
	byte       *dest;
	byte       *buf = tex->data;
	const unsigned char *p;
	int         width = tex->width;

	dest = buf + ((y * width) + x);

	p = (const unsigned char *) s;
	while (*p) {
		SCR_DrawCharToSnap (*p++, dest, width);
		dest += 8;
	}
}

void
SCR_Init (void)
{
	// register our commands
	Cmd_AddCommand ("screenshot", SCR_ScreenShot_f, "Take a screenshot, "
					"saves as qfxxx.pcx in the current directory");
	Cmd_AddCommand ("sizeup", SCR_SizeUp_f, "Increases the screen size");
	Cmd_AddCommand ("sizedown", SCR_SizeDown_f, "Decreases the screen size");

	scr_ram = Draw_PicFromWad ("ram");
	scr_net = Draw_PicFromWad ("net");
	scr_turtle = Draw_PicFromWad ("turtle");

	scr_initialized = true;
}
