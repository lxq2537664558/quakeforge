/*
	gl_lightmap.c

	surface-related refresh code

	Copyright (C) 1996-1997  Id Software, Inc.
	Copyright (C) 2000       Joseph Carter <knghtbrd@debian.org>

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

#include <math.h>
#include <stdio.h>

#include "QF/cvar.h"
#include "QF/render.h"
#include "QF/sys.h"
#include "QF/GL/defines.h"
#include "QF/GL/funcs.h"
#include "QF/GL/qf_lightmap.h"
#include "QF/GL/qf_rmain.h"
#include "QF/GL/qf_sky.h"
#include "QF/GL/qf_textures.h"
#include "QF/GL/qf_vid.h"

#include "compat.h"
#include "r_cvar.h"
#include "r_local.h"
#include "r_shared.h"

int          active_lightmaps;
int          dlightdivtable[8192];
int			 gl_internalformat;				// 1 or 3
int          lightmap_bytes;				// 1, 3, or 4
int          lightmap_textures;

// keep lightmap texture data in main memory so texsubimage can update properly
// LordHavoc: changed to be allocated at runtime (typically lower memory usage)
byte        *lightmaps[MAX_LIGHTMAPS];

unsigned int blocklights[34 * 34 * 3];	//FIXME make dynamic
int          allocated[MAX_LIGHTMAPS][BLOCK_WIDTH];

qboolean	 lightmap_modified[MAX_GLTEXTURES];
glpoly_t	*lightmap_polys[MAX_LIGHTMAPS];
glRect_t	 lightmap_rectchange[MAX_LIGHTMAPS];

void (*R_BuildLightMap) (msurface_t *surf);


void
gl_lightmap_init (void)
{
	int         s;

	memset (&lightmaps, 0, sizeof (lightmaps));
	dlightdivtable[0] = 1048576 >> 7;
	for (s = 1; s < 8192; s++)
		dlightdivtable[s] = 1048576 / (s << 7);
}

static void
R_RecursiveLightUpdate (mnode_t *node)
{
	int         c;
	msurface_t *surf;

	if (node->children[0]->contents >= 0)
		R_RecursiveLightUpdate (node->children[0]);
	if (node->children[1]->contents >= 0)
		R_RecursiveLightUpdate (node->children[1]);
	if ((c = node->numsurfaces))
		for (surf = r_worldentity.model->surfaces + node->firstsurface; c;
			 c--, surf++)
			surf->cached_dlight = true;
}

static inline void
R_AddDynamicLights_1 (msurface_t *surf)
{
	float			dist;
	int				maxdist, maxdist2, maxdist3, smax, smax_bytes, tmax,
					grey, s, t;
	unsigned int	lnum, td, i, j;
	unsigned int    sdtable[18];
	unsigned int   *bl;
	vec3_t			impact, local;

	smax = (surf->extents[0] >> 4) + 1;
	smax_bytes = smax * gl_internalformat;
	tmax = (surf->extents[1] >> 4) + 1;

	for (lnum = 0; lnum < r_maxdlights; lnum++) {
		if (!(surf->dlightbits & (1 << lnum)))
			continue;					// not lit by this light

		VectorSubtract (r_dlights[lnum].origin, currententity->origin, local);
		dist = DotProduct (local, surf->plane->normal) - surf->plane->dist;
		VectorMultSub (r_dlights[lnum].origin, dist, surf->plane->normal,
					   impact);

		i = DotProduct (impact,	surf->texinfo->vecs[0]) +
			surf->texinfo->vecs[0][3] - surf->texturemins[0];

		// reduce calculations
		t = dist * dist;
		for (s = 0; s < smax; s++, i -= 16)
			sdtable[s] = i * i + t;

		i = DotProduct (impact,	surf->texinfo->vecs[1]) +
			surf->texinfo->vecs[1][3] - surf->texturemins[1];

		// for comparisons to minimum acceptable light
		maxdist = (int) ((r_dlights[lnum].radius * r_dlights[lnum].radius) *
						 0.75);

		// clamp radius to avoid exceeding 8192 entry division table
		if (maxdist > 1048576)
			maxdist = 1048576;
		maxdist3 = maxdist - t;

		// convert to 8.8 blocklights format
		grey = (r_dlights[lnum].color[0] + r_dlights[lnum].color[1] +
				r_dlights[lnum].color[2]) * maxdist / 3.0;
		bl = blocklights;
		for (t = 0; t < tmax; t++, i -= 16) {
			td = i * i;
			if (td < maxdist3) {		// ensure part is visible on this line
				maxdist2 = maxdist - td;
				for (s = 0; s < smax; s++) {
					if (sdtable[s] < maxdist2) {
						j = dlightdivtable[(sdtable[s] + td) >> 7];
						*bl++ += (grey * j) >> 7;
					} else
						bl++;
				}
			} else
				bl += smax_bytes;		// skip line
		}
	}
}

static inline void
R_AddDynamicLights_3 (msurface_t *surf)
{
	float			dist;
	int				maxdist, maxdist2, maxdist3, smax, smax_bytes, tmax,
					red, green, blue, s, t;
	unsigned int	lnum, td, i, j;
	unsigned int    sdtable[18];
	unsigned int   *bl;
	vec3_t			impact, local;

	smax = (surf->extents[0] >> 4) + 1;
	smax_bytes = smax * gl_internalformat;
	tmax = (surf->extents[1] >> 4) + 1;

	for (lnum = 0; lnum < r_maxdlights; lnum++) {
		if (!(surf->dlightbits & (1 << lnum)))
			continue;					// not lit by this light

		VectorSubtract (r_dlights[lnum].origin, currententity->origin, local);
		dist = DotProduct (local, surf->plane->normal) - surf->plane->dist;
		VectorMultSub (r_dlights[lnum].origin, dist, surf->plane->normal,
					   impact);

		i = DotProduct (impact,	surf->texinfo->vecs[0]) +
			surf->texinfo->vecs[0][3] - surf->texturemins[0];

		// reduce calculations
		t = dist * dist;
		for (s = 0; s < smax; s++, i -= 16)
			sdtable[s] = i * i + t;

		i = DotProduct (impact,	surf->texinfo->vecs[1]) +
			surf->texinfo->vecs[1][3] - surf->texturemins[1];

		// for comparisons to minimum acceptable light
		maxdist = (int) ((r_dlights[lnum].radius * r_dlights[lnum].radius) *
						 0.75);

		// clamp radius to avoid exceeding 8192 entry division table
		if (maxdist > 1048576)
			maxdist = 1048576;
		maxdist3 = maxdist - t;

		// convert to 8.8 blocklights format
		red = r_dlights[lnum].color[0] * maxdist;
		green = r_dlights[lnum].color[1] * maxdist;
		blue = r_dlights[lnum].color[2] * maxdist;
		bl = blocklights;
		for (t = 0; t < tmax; t++, i -= 16) {
			td = i * i;
			if (td < maxdist3) {		// ensure part is visible on this line
				maxdist2 = maxdist - td;
				for (s = 0; s < smax; s++) {
					if (sdtable[s] < maxdist2) {
						j = dlightdivtable[(sdtable[s] + td) >> 7];
						*bl++ += (red * j) >> 7;
						*bl++ += (green * j) >> 7;
						*bl++ += (blue * j) >> 7;
					} else
						bl += 3;
				}
			} else
				bl += smax_bytes;		// skip line
		}
	}
}

static void
R_BuildLightMap_1 (msurface_t *surf)
{
	byte		   *dest;
	int				maps, size, stride, smax, tmax, i, j;
	unsigned int	scale;
	unsigned int   *bl;

	surf->cached_dlight = (surf->dlightframe == r_framecount);

	smax = (surf->extents[0] >> 4) + 1;
	tmax = (surf->extents[1] >> 4) + 1;
	size = smax * tmax * gl_internalformat;

	// set to full bright if no light data
	if (!r_worldentity.model->lightdata) {
		memset (&blocklights[0], 0xff, size * sizeof(int));
		goto store;
	}

	// clear to no light
	memset (&blocklights[0], 0, size * sizeof(int));

	// add all the lightmaps
	if (surf->samples) {
		byte		   *lightmap;

		lightmap = surf->samples;
		for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255;
			 maps++) {
			scale = d_lightstylevalue[surf->styles[maps]];
			surf->cached_light[maps] = scale;					// 8.8 fraction
			bl = blocklights;
			for (i = 0; i < size; i++) {
				*bl++ += *lightmap++ * scale;
			}
		}
	}
	// add all the dynamic lights
	if (surf->dlightframe == r_framecount)
		R_AddDynamicLights_1 (surf);

  store:
	// bound and shift
	// Also, invert because we're using a diff blendfunc now

	stride = (BLOCK_WIDTH - smax) * lightmap_bytes;
	bl = blocklights;

	dest = lightmaps[surf->lightmaptexturenum]
			+ (surf->light_t * BLOCK_WIDTH + surf->light_s) * lightmap_bytes;

	switch (gl_overbright->int_val) {
	case 2:
		for (i = 0; i < tmax; i++, dest += stride) {
			for (j = smax; j; j--) {
				*dest++ = min (*bl >> 9, 255);
				bl++;
			}
		}
		break;
	case 1:
		for (i = 0; i < tmax; i++, dest += stride) {
			for (j = smax; j; j--) {
				*dest++ = min ((*bl >> 9) + (*bl >> 10), 255);
				bl++;
			}
		}
		break;
	default:
		for (i = 0; i < tmax; i++, dest += stride) {
			for (j = smax; j; j--) {
				*dest++ = min (*bl >> 8, 255);
				bl++;
			}
		}
		break;
	}
}

static void
R_BuildLightMap_3 (msurface_t *surf)
{
	byte		   *dest;
	int				maps, size, stride, smax, tmax, i, j;
	unsigned int	scale;
	unsigned int   *bl;

	surf->cached_dlight = (surf->dlightframe == r_framecount);

	smax = (surf->extents[0] >> 4) + 1;
	tmax = (surf->extents[1] >> 4) + 1;
	size = smax * tmax * gl_internalformat;

	// set to full bright if no light data
	if (!r_worldentity.model->lightdata) {
		memset (&blocklights[0], 0xff, size * sizeof(int));
		goto store;
	}

	// clear to no light
	memset (&blocklights[0], 0, size * sizeof(int));

	// add all the lightmaps
	if (surf->samples) {
		byte		   *lightmap;

		lightmap = surf->samples;
		for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255;
			 maps++) {
			scale = d_lightstylevalue[surf->styles[maps]];
			surf->cached_light[maps] = scale;					// 8.8 fraction
			bl = blocklights;
			for (i = 0; i < smax * tmax; i++) {
				*bl++ += *lightmap++ * scale;
				*bl++ += *lightmap++ * scale;
				*bl++ += *lightmap++ * scale;
			}
		}
	}
	// add all the dynamic lights
	if (surf->dlightframe == r_framecount)
		R_AddDynamicLights_3 (surf);

  store:
	// bound and shift
	// and invert too
	stride = (BLOCK_WIDTH - smax) * lightmap_bytes;
	bl = blocklights;

	dest = lightmaps[surf->lightmaptexturenum]
			+ (surf->light_t * BLOCK_WIDTH + surf->light_s) * lightmap_bytes;

	switch (gl_overbright->int_val) {
	case 2:
		for (i = 0; i < tmax; i++, dest += stride) {
			for (j = 0; j < smax; j++) {
				*dest++ = min (*bl >> 9, 255);
				bl++;
				*dest++ = min (*bl >> 9, 255);
				bl++;
				*dest++ = min (*bl >> 9, 255);
				bl++;
			}
		}
		break;
	case 1:
		for (i = 0; i < tmax; i++, dest += stride) {
			for (j = 0; j < smax; j++) {
				*dest++ = min ((*bl >> 9) + (*bl >> 10), 255);
				bl++;
				*dest++ = min ((*bl >> 9) + (*bl >> 10), 255);
				bl++;
				*dest++ = min ((*bl >> 9) + (*bl >> 10), 255);
				bl++;
			}
		}
		break;
	default:
		for (i = 0; i < tmax; i++, dest += stride) {
			for (j = 0; j < smax; j++) {
				*dest++ = min (*bl >> 8, 255);
				bl++;
				*dest++ = min (*bl >> 8, 255);
				bl++;
				*dest++ = min (*bl >> 8, 255);
				bl++;
			}
		}
		break;
	}
}

static void
R_BuildLightMap_4 (msurface_t *surf)
{
	byte		   *dest;
	int				maps, size, smax, tmax, i, j, stride;
	unsigned int	scale;
	unsigned int   *bl;

	surf->cached_dlight = (surf->dlightframe == r_framecount);

	smax = (surf->extents[0] >> 4) + 1;
	tmax = (surf->extents[1] >> 4) + 1;
	size = smax * tmax * gl_internalformat;

	// set to full bright if no light data
	if (!r_worldentity.model->lightdata) {
		memset (&blocklights[0], 0xff, size * sizeof(int));
		goto store;
	}

	// clear to no light
	memset (&blocklights[0], 0, size * sizeof(int));

	// add all the lightmaps
	if (surf->samples) {
		byte		   *lightmap;

		lightmap = surf->samples;
		for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255;
			 maps++) {
			scale = d_lightstylevalue[surf->styles[maps]];
			surf->cached_light[maps] = scale;					// 8.8 fraction
			bl = blocklights;
			for (i = 0; i < smax * tmax; i++) {
				*bl++ += *lightmap++ * scale;
				*bl++ += *lightmap++ * scale;
				*bl++ += *lightmap++ * scale;
			}
		}
	}
	// add all the dynamic lights
	if (surf->dlightframe == r_framecount)
		R_AddDynamicLights_3 (surf);

  store:
	// bound and shift
	// and invert too
	stride = (BLOCK_WIDTH - smax) * lightmap_bytes;
	bl = blocklights;

	dest = lightmaps[surf->lightmaptexturenum]
			+ (surf->light_t * BLOCK_WIDTH + surf->light_s) * lightmap_bytes;

	switch (gl_overbright->int_val) {
	case 2:
		for (i = 0; i < tmax; i++, dest += stride) {
			for (j = 0; j < smax; j++) {
				*dest++ = min (*bl >> 9, 255);
				bl++;
				*dest++ = min (*bl >> 9, 255);
				bl++;
				*dest++ = min (*bl >> 9, 255);
				bl++;
				*dest++ = 255;
			}
		}
		break;
	case 1:
		for (i = 0; i < tmax; i++, dest += stride) {
			for (j = 0; j < smax; j++) {
				*dest++ = min ((*bl >> 9) + (*bl >> 10), 255);
				bl++;
				*dest++ = min ((*bl >> 9) + (*bl >> 10), 255);
				bl++;
				*dest++ = min ((*bl >> 9) + (*bl >> 10), 255);
				bl++;
				*dest++ = 255;
			}
		}
		break;
	default:
		for (i = 0; i < tmax; i++, dest += stride) {
			for (j = 0; j < smax; j++) {
				*dest++ = min (*bl >> 8, 255);
				bl++;
				*dest++ = min (*bl >> 8, 255);
				bl++;
				*dest++ = min (*bl >> 8, 255);
				bl++;
				*dest++ = 255;
			}
		}
		break;
	}
}

// BRUSH MODELS ===============================================================

static void
GL_UploadLightmap (int i)
{
	switch (gl_lightmap_subimage->int_val) {
	case 2:
		qfglTexSubImage2D (GL_TEXTURE_2D, 0, lightmap_rectchange[i].l,
						   lightmap_rectchange[i].t, lightmap_rectchange[i].w,
						   lightmap_rectchange[i].h, gl_lightmap_format,
						   GL_UNSIGNED_BYTE, lightmaps[i] +
						   (lightmap_rectchange[i].t * BLOCK_WIDTH +
							lightmap_rectchange[i].l) * lightmap_bytes);
		break;
	case 1:
		qfglTexSubImage2D (GL_TEXTURE_2D, 0, 0, lightmap_rectchange[i].t,
						   BLOCK_WIDTH, lightmap_rectchange[i].h,
						   gl_lightmap_format, GL_UNSIGNED_BYTE,
						   lightmaps[i] + (lightmap_rectchange[i].t *
										   BLOCK_WIDTH) * lightmap_bytes);
		break;
	default:
	case 0:
		qfglTexImage2D (GL_TEXTURE_2D, 0, gl_internalformat, BLOCK_WIDTH,
						BLOCK_HEIGHT, 0, gl_lightmap_format, GL_UNSIGNED_BYTE,
						lightmaps[i]);
		break;
	}
}

void
R_CalcLightmaps (void)
{
	int         i;
	glpoly_t   *p;

	for (i = 0; i < MAX_LIGHTMAPS; i++) {
		p = lightmap_polys[i];
		if (!p)
			continue;
		if (lightmap_modified[i]) {
			qfglBindTexture (GL_TEXTURE_2D, lightmap_textures + i);
			GL_UploadLightmap (i);
			lightmap_modified[i] = false;
		}
	}
}

void
R_BlendLightmaps (void)
{
	float      *v;
	int         i, j;
	glpoly_t   *p;

	qfglDepthMask (GL_FALSE);					// don't bother writing Z

	if (gl_doublebright->int_val)
		qfglBlendFunc (GL_DST_COLOR, GL_SRC_COLOR);
	else
		qfglBlendFunc (GL_ZERO, GL_SRC_COLOR);

	for (i = 0; i < MAX_LIGHTMAPS; i++) {
		p = lightmap_polys[i];
		if (!p)
			continue;
		qfglBindTexture (GL_TEXTURE_2D, lightmap_textures + i);
		for (; p; p = p->chain) {
			qfglBegin (GL_POLYGON);
			v = p->verts[0];
			for (j = 0; j < p->numverts; j++, v += VERTEXSIZE) {
				qfglTexCoord2fv (&v[5]);
				qfglVertex3fv (v);
			}
			qfglEnd ();
		}
	}

	// Return to normal blending  --KB
	qfglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qfglDepthMask (GL_TRUE);					// back to normal Z buffering
}

void
gl_overbright_f (cvar_t *var)
{
	int			 num, i, j;
	model_t		*m;
	msurface_t  *fa;

	if (R_BuildLightMap == 0)
		return;

	for (i = 0; i < r_numvisedicts; i++) {
		m = r_visedicts[i]->model;

		if (m->type != mod_brush)
			continue;
		if (m->name[0] == '*')
			continue;

		for (j = 0, fa = m->surfaces; j < m->numsurfaces; j++, fa++) {
			if (fa->flags & (SURF_DRAWTURB | SURF_DRAWSKY))
				continue;
			num = fa->lightmaptexturenum;

			lightmap_modified[num] = true;
			lightmap_rectchange[num].l = 0;
			lightmap_rectchange[num].t = 0;
			lightmap_rectchange[num].w = BLOCK_WIDTH;
			lightmap_rectchange[num].h = BLOCK_HEIGHT;
			R_BuildLightMap (fa);
		}
	}

	m = r_worldentity.model;

	for (i = 0, fa = m->surfaces; i < m->numsurfaces; i++, fa++) {
		if (fa->flags & (SURF_DRAWTURB | SURF_DRAWSKY))
			continue;

		num = fa->lightmaptexturenum;
		lightmap_modified[num] = true;
		lightmap_rectchange[num].l = 0;
		lightmap_rectchange[num].t = 0;
		lightmap_rectchange[num].w = BLOCK_WIDTH;
		lightmap_rectchange[num].h = BLOCK_HEIGHT;

		R_BuildLightMap (fa);
	}
}

void
R_CalcAndBlendLightmaps (void)
{
	float      *v;
	int         i, j;
	glpoly_t   *p;

	qfglDepthMask (GL_FALSE);					// don't bother writing Z
	if (gl_doublebright->int_val)
		qfglBlendFunc (GL_DST_COLOR, GL_SRC_COLOR);
	else
		qfglBlendFunc (GL_ZERO, GL_SRC_COLOR);

	for (i = 0; i < MAX_LIGHTMAPS; i++) {
		p = lightmap_polys[i];
		if (!p)
			continue;
		if (lightmap_modified[i]) {
			qfglBindTexture (GL_TEXTURE_2D, lightmap_textures + i);
			GL_UploadLightmap (i);
			lightmap_modified[i] = false;
		}
		for (; p; p = p->chain) {
			qfglBegin (GL_POLYGON);
			v = p->verts[0];
			for (j = 0; j < p->numverts; j++, v += VERTEXSIZE) {
				qfglTexCoord2fv (&v[5]);
				qfglVertex3fv (v);
			}
			qfglEnd ();
		}
	}

	// Return to normal blending  --KB
	qfglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qfglDepthMask (GL_TRUE);					// back to normal Z buffering
}

// LIGHTMAP ALLOCATION ========================================================

// returns a texture number and the position inside it
static int
AllocBlock (int w, int h, int *x, int *y)
{
	int         best, best2, texnum, i, j;

	for (texnum = 0; texnum < MAX_LIGHTMAPS; texnum++) {
		best = BLOCK_HEIGHT;

		for (i = 0; i < BLOCK_WIDTH - w; i++) {
			best2 = 0;

			for (j = 0; j < w; j++) {
				if (allocated[texnum][i + j] >= best)
					break;
				if (allocated[texnum][i + j] > best2)
					best2 = allocated[texnum][i + j];
			}
			if (j == w) {
				// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
			continue;

		// LordHavoc: allocate lightmaps only as needed
		if (!lightmaps[texnum])
			lightmaps[texnum] = calloc (BLOCK_WIDTH * BLOCK_HEIGHT,
										lightmap_bytes);
		for (i = 0; i < w; i++)
			allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	Sys_Error ("AllocBlock: full");
	return 0;
}

static void
GL_CreateSurfaceLightmap (msurface_t *surf)
{
	int         smax, tmax;

	if (surf->flags & (SURF_DRAWSKY | SURF_DRAWTURB))
		return;

	smax = (surf->extents[0] >> 4) + 1;
	tmax = (surf->extents[1] >> 4) + 1;

	surf->lightmaptexturenum =
		AllocBlock (smax, tmax, &surf->light_s, &surf->light_t);
	R_BuildLightMap (surf);
}

/*
  GL_BuildLightmaps

  Builds the lightmap texture with all the surfaces from all brush models
*/
void
GL_BuildLightmaps (model_t **models, int num_models)
{
	int         i, j;
	model_t    *m;

	memset (allocated, 0, sizeof (allocated));

	r_framecount = 1;					// no dlightcache

	if (!lightmap_textures) {
		lightmap_textures = texture_extension_number;
		texture_extension_number += MAX_LIGHTMAPS;
	}

	switch (r_lightmap_components->int_val) {
	case 1:
		gl_internalformat = 1;
		gl_lightmap_format = GL_LUMINANCE;
		lightmap_bytes = 1;
		R_BuildLightMap = R_BuildLightMap_1;
		break;
	case 3:
		gl_internalformat = 3;
		if (use_bgra)
			gl_lightmap_format = GL_BGR;
		else
			gl_lightmap_format = GL_RGB;
		lightmap_bytes = 3;
		R_BuildLightMap = R_BuildLightMap_3;
		break;
	case 4:
	default:
		gl_internalformat = 3;
		if (use_bgra)
			gl_lightmap_format = GL_BGRA;
		else
			gl_lightmap_format = GL_RGBA;
		lightmap_bytes = 4;
		R_BuildLightMap = R_BuildLightMap_4;
		break;
	}

	for (j = 1; j < num_models; j++) {
		m = models[j];
		if (!m)
			break;
		if (m->name[0] == '*')
			continue;
		r_pcurrentvertbase = m->vertexes;
		currentmodel = m;
		for (i = 0; i < m->numsurfaces; i++) {
			if (m->surfaces[i].flags & SURF_DRAWTURB)
				continue;
			if (gl_sky_divide->int_val && (m->surfaces[i].flags &
										   SURF_DRAWSKY))
				continue;
			GL_CreateSurfaceLightmap (m->surfaces + i);
			BuildSurfaceDisplayList (m->surfaces + i);
		}
	}

	// upload all lightmaps that were filled
	for (i = 0; i < MAX_LIGHTMAPS; i++) {
		if (!allocated[i][0])
			break;						// no more used
		lightmap_modified[i] = false;
		lightmap_rectchange[i].l = BLOCK_WIDTH;
		lightmap_rectchange[i].t = BLOCK_HEIGHT;
		lightmap_rectchange[i].w = 0;
		lightmap_rectchange[i].h = 0;
		qfglBindTexture (GL_TEXTURE_2D, lightmap_textures + i);
		qfglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qfglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		qfglTexImage2D (GL_TEXTURE_2D, 0, lightmap_bytes, BLOCK_WIDTH,
						BLOCK_HEIGHT, 0, gl_lightmap_format,
						GL_UNSIGNED_BYTE, lightmaps[i]);
	}
}
