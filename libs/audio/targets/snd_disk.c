/*
	snd_disk.c

	write sound to a disk file

	Copyright (C) 1999,2000  contributors of the QuakeForge project
	Please see the file "AUTHORS" for a list of contributors

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

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "QF/cvar.h"
#include "QF/plugin.h"
#include "QF/qargs.h"
#include "QF/sound.h"
#include "QF/sys.h"

static int  snd_inited;
static QFile      *snd_file;
static int snd_blocked = 0;
//FIXME static volatile dma_t sn;

static plugin_t				plugin_info;
static plugin_data_t		plugin_info_data;
static plugin_funcs_t		plugin_info_funcs;
static general_data_t		plugin_info_general_data;
static general_funcs_t		plugin_info_general_funcs;
static snd_output_data_t	plugin_info_snd_output_data;
static snd_output_funcs_t	plugin_info_snd_output_funcs;


/* FIXME
static qboolean
SNDDMA_Init (void)
{
	shm = &sn;
	memset ((dma_t *) shm, 0, sizeof (*shm));
	shm->splitbuffer = 0;
	shm->channels = 2;
	shm->submission_chunk = 1;			// don't mix less than this #
	shm->samplepos = 0;					// in mono samples
	shm->samplebits = 16;
	shm->samples = 16384;				// mono samples in buffer
	shm->speed = 44100;
	shm->buffer = malloc (shm->samples * shm->channels * shm->samplebits / 8);
	if (!shm->buffer) {
		Sys_Printf ("SNDDMA_Init: memory allocation failure\n");
		return 0;
	}

	Sys_Printf ("%5d stereo\n", shm->channels - 1);
	Sys_Printf ("%5d samples\n", shm->samples);
	Sys_Printf ("%5d samplepos\n", shm->samplepos);
	Sys_Printf ("%5d samplebits\n", shm->samplebits);
	Sys_Printf ("%5d submission_chunk\n", shm->submission_chunk);
	Sys_Printf ("%5d speed\n", shm->speed);
	Sys_Printf ("0x%x dma buffer\n", (int) shm->buffer);

	if (!(snd_file = Qopen ("qf.raw", "wb")))
		return 0;

	snd_inited = 1;
	return 1;
}
*/

static int
SNDDMA_GetDMAPos (void)
{
	shm->samplepos = 0;
	return shm->samplepos;
}

static void
SNDDMA_Shutdown (void)
{
	if (snd_inited) {
		Qclose (snd_file);
		snd_file = 0;
		free (shm->buffer);
		snd_inited = 0;
	}
}

/*
	SNDDMA_Submit

	Send sound to device if buffer isn't really the dma buffer
*/
static void
SNDDMA_Submit (void)
{
	int		count = ((*plugin_info_snd_output_data.paintedtime -
					  *plugin_info_snd_output_data.soundtime) *
					 shm->samplebits / 8);

	if (snd_blocked)
		return;

	Qwrite (snd_file, shm->buffer, count);
}

static void
SNDDMA_BlockSound (void)                
{
	++snd_blocked;
}   
    
static void
SNDDMA_UnblockSound (void)
{
	if (!snd_blocked)
		return;
	--snd_blocked;
}

QFPLUGIN plugin_t *PLUGIN_INFO(snd_output, disk) (void);
QFPLUGIN plugin_t *
PLUGIN_INFO(snd_output, disk) (void) {
	plugin_info.type = qfp_snd_output;
	plugin_info.api_version = QFPLUGIN_VERSION;
	plugin_info.plugin_version = "0.1";
	plugin_info.description = "disk output";
	plugin_info.copyright = "Copyright (C) 1996-1997 id Software, Inc.\n"
		"Copyright (C) 1999,2000,2001  contributors of the QuakeForge "
		"project\n"
		"Please see the file \"AUTHORS\" for a list of contributors";
	plugin_info.functions = &plugin_info_funcs;
	plugin_info.data = &plugin_info_data;

	plugin_info_data.general = &plugin_info_general_data;
	plugin_info_data.input = NULL;
	plugin_info_data.snd_output = &plugin_info_snd_output_data;

	plugin_info_funcs.general = &plugin_info_general_funcs;
	plugin_info_funcs.input = NULL;
	plugin_info_funcs.snd_output = &plugin_info_snd_output_funcs;

//	plugin_info_general_funcs.p_Init = SNDDMA_Init; // FIXME
	plugin_info_general_funcs.p_Shutdown = SNDDMA_Shutdown;
	plugin_info_snd_output_funcs.pS_O_GetDMAPos = SNDDMA_GetDMAPos;
	plugin_info_snd_output_funcs.pS_O_Submit = SNDDMA_Submit;
	plugin_info_snd_output_funcs.pS_O_BlockSound = SNDDMA_BlockSound;
	plugin_info_snd_output_funcs.pS_O_UnblockSound = SNDDMA_UnblockSound;

	return &plugin_info;
}
