/*
	sv_progs.c

	Quick QuakeC server code

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

	$Id$
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_STRING_H
#include "string.h"
#endif
#ifdef HAVE_STRINGS_H
#include "strings.h"
#endif

#include "cmd.h"
#include "console.h"
#include "host.h"
#include "progdefs.h"
#include "server.h"
#include "sv_progs.h"
#include "world.h"

progs_t     sv_pr_state;
sv_globals_t sv_globals;
sv_funcs_t sv_funcs;
sv_fields_t sv_fields;

cvar_t     *r_skyname;
cvar_t     *sv_progs;
cvar_t     *pr_checkextentions;

cvar_t     *nomonsters;
cvar_t     *gamecfg;
cvar_t     *scratch1;
cvar_t     *scratch2;
cvar_t     *scratch3;
cvar_t     *scratch4;
cvar_t     *savedgamecfg;
cvar_t     *saved1;
cvar_t     *saved2;
cvar_t     *saved3;
cvar_t     *saved4;

int
ED_Prune_Edict (progs_t *pr, edict_t *ent)
{
	// remove things from different skill levels or deathmatch
	if (deathmatch->int_val) {
		if (((int) SVFIELD (ent, spawnflags, float) & SPAWNFLAG_NOT_DEATHMATCH)) {
			return 1;
		}
	} else if (
			   (current_skill == 0
				&& ((int) SVFIELD (ent, spawnflags, float) & SPAWNFLAG_NOT_EASY))
			   || (current_skill == 1
				   && ((int) SVFIELD (ent, spawnflags, float) & SPAWNFLAG_NOT_MEDIUM))
			   || (current_skill >= 2
				   && ((int) SVFIELD (ent, spawnflags, float) & SPAWNFLAG_NOT_HARD))) {
		return 1;
	}
	return 0;
}

void
ED_PrintEdicts_f (void)
{
	ED_PrintEdicts (&sv_pr_state);
}

/*
	ED_PrintEdict_f

	For debugging, prints a single edicy
*/
void
ED_PrintEdict_f (void)
{
	int         i;

	i = atoi (Cmd_Argv (1));
	Con_Printf ("\n EDICT %i:\n", i);
	ED_PrintNum (&sv_pr_state, i);
}

void
ED_Count_f (void)
{
	ED_Count (&sv_pr_state);
}

void
PR_Profile_f (void)
{
	PR_Profile (&sv_pr_state);
}

int
ED_Parse_Extra_Fields (progs_t * pr, char *key, char *value)
{
	return 0;
}

void
SV_LoadProgs (void)
{
	PR_LoadProgs (&sv_pr_state, sv_progs->string);
	if (!sv_pr_state.progs)
		Host_Error ("SV_LoadProgs: couldn't load %s", sv_progs->string);
	if (sv_pr_state.progs->crc != PROGHEADER_CRC)
		Host_Error ("You must have the progs.dat from Quake installed");
	// progs engine needs these globals anyway
	sv_globals.self = sv_pr_state.globals.self;
	sv_globals.time = sv_pr_state.globals.time;

(void *) sv_globals.self = PR_GetGlobalPointer (&sv_pr_state, "self");
(void *) sv_globals.other = PR_GetGlobalPointer (&sv_pr_state, "other");
(void *) sv_globals.world = PR_GetGlobalPointer (&sv_pr_state, "world");
(void *) sv_globals.time = PR_GetGlobalPointer (&sv_pr_state, "time");
(void *) sv_globals.frametime = PR_GetGlobalPointer (&sv_pr_state, "frametime");
(void *) sv_globals.force_retouch = PR_GetGlobalPointer (&sv_pr_state, "force_retouch");
(void *) sv_globals.mapname = PR_GetGlobalPointer (&sv_pr_state, "mapname");
(void *) sv_globals.deathmatch = PR_GetGlobalPointer (&sv_pr_state, "deathmatch");
(void *) sv_globals.coop = PR_GetGlobalPointer (&sv_pr_state, "coop");
(void *) sv_globals.teamplay = PR_GetGlobalPointer (&sv_pr_state, "teamplay");
(void *) sv_globals.serverflags = PR_GetGlobalPointer (&sv_pr_state, "serverflags");
(void *) sv_globals.total_secrets = PR_GetGlobalPointer (&sv_pr_state, "total_secrets");
(void *) sv_globals.total_monsters = PR_GetGlobalPointer (&sv_pr_state, "total_monsters");
(void *) sv_globals.found_secrets = PR_GetGlobalPointer (&sv_pr_state, "found_secrets");
(void *) sv_globals.killed_monsters = PR_GetGlobalPointer (&sv_pr_state, "killed_monsters");
(void *) sv_globals.parms = PR_GetGlobalPointer (&sv_pr_state, "parm1");
(void *) sv_globals.v_forward = PR_GetGlobalPointer (&sv_pr_state, "v_forward");
(void *) sv_globals.v_up = PR_GetGlobalPointer (&sv_pr_state, "v_up");
(void *) sv_globals.v_right = PR_GetGlobalPointer (&sv_pr_state, "v_right");
(void *) sv_globals.trace_allsolid = PR_GetGlobalPointer (&sv_pr_state, "trace_allsolid");
(void *) sv_globals.trace_startsolid = PR_GetGlobalPointer (&sv_pr_state, "trace_startsolid");
(void *) sv_globals.trace_fraction = PR_GetGlobalPointer (&sv_pr_state, "trace_fraction");
(void *) sv_globals.trace_endpos = PR_GetGlobalPointer (&sv_pr_state, "trace_endpos");
(void *) sv_globals.trace_plane_normal = PR_GetGlobalPointer (&sv_pr_state, "trace_plane_normal");
(void *) sv_globals.trace_plane_dist = PR_GetGlobalPointer (&sv_pr_state, "trace_plane_dist");
(void *) sv_globals.trace_ent = PR_GetGlobalPointer (&sv_pr_state, "trace_ent");
(void *) sv_globals.trace_inopen = PR_GetGlobalPointer (&sv_pr_state, "trace_inopen");
(void *) sv_globals.trace_inwater = PR_GetGlobalPointer (&sv_pr_state, "trace_inwater");
(void *) sv_globals.msg_entity = PR_GetGlobalPointer (&sv_pr_state, "msg_entity");

#ifdef QUAKE2
(void *) sv_globals.startspot = PR_GetGlobalPointer (&sv_pr_state, "startspot");
(void *) sv_globals.null = PR_GetGlobalPointer (&sv_pr_state, "null");
#endif

sv_funcs.main = PR_GetFunctionIndex (&sv_pr_state, "main");
sv_funcs.StartFrame = PR_GetFunctionIndex (&sv_pr_state, "StartFrame");
sv_funcs.PlayerPreThink = PR_GetFunctionIndex (&sv_pr_state, "PlayerPreThink");
sv_funcs.PlayerPostThink = PR_GetFunctionIndex (&sv_pr_state, "PlayerPostThink");
sv_funcs.ClientKill = PR_GetFunctionIndex (&sv_pr_state, "ClientKill");
sv_funcs.ClientConnect = PR_GetFunctionIndex (&sv_pr_state, "ClientConnect");
sv_funcs.PutClientInServer = PR_GetFunctionIndex (&sv_pr_state, "PutClientInServer");
sv_funcs.ClientDisconnect = PR_GetFunctionIndex (&sv_pr_state, "ClientDisconnect");
sv_funcs.SetNewParms = PR_GetFunctionIndex (&sv_pr_state, "SetNewParms");
sv_funcs.SetChangeParms = PR_GetFunctionIndex (&sv_pr_state, "SetChangeParms");

sv_fields.modelindex = ED_GetFieldIndex (&sv_pr_state, "modelindex");
sv_fields.absmin = ED_GetFieldIndex (&sv_pr_state, "absmin");
sv_fields.absmax = ED_GetFieldIndex (&sv_pr_state, "absmax");
sv_fields.ltime = ED_GetFieldIndex (&sv_pr_state, "ltime");
sv_fields.movetype = ED_GetFieldIndex (&sv_pr_state, "movetype");
sv_fields.solid = ED_GetFieldIndex (&sv_pr_state, "solid");
sv_fields.origin = ED_GetFieldIndex (&sv_pr_state, "origin");
sv_fields.oldorigin = ED_GetFieldIndex (&sv_pr_state, "oldorigin");
sv_fields.velocity = ED_GetFieldIndex (&sv_pr_state, "velocity");
sv_fields.angles = ED_GetFieldIndex (&sv_pr_state, "angles");
sv_fields.avelocity = ED_GetFieldIndex (&sv_pr_state, "avelocity");
sv_fields.basevelocity = ED_GetFieldIndex (&sv_pr_state, "basevelocity");
sv_fields.punchangle = ED_GetFieldIndex (&sv_pr_state, "punchangle");
sv_fields.classname = ED_GetFieldIndex (&sv_pr_state, "classname");
sv_fields.model = ED_GetFieldIndex (&sv_pr_state, "model");
sv_fields.frame = ED_GetFieldIndex (&sv_pr_state, "frame");
sv_fields.skin = ED_GetFieldIndex (&sv_pr_state, "skin");
sv_fields.effects = ED_GetFieldIndex (&sv_pr_state, "effects");
sv_fields.drawPercent = ED_GetFieldIndex (&sv_pr_state, "drawPercent");
sv_fields.gravity = ED_GetFieldIndex (&sv_pr_state, "gravity");
sv_fields.mass = ED_GetFieldIndex (&sv_pr_state, "mass");
sv_fields.light_level = ED_GetFieldIndex (&sv_pr_state, "light_level");
sv_fields.mins = ED_GetFieldIndex (&sv_pr_state, "mins");
sv_fields.maxs = ED_GetFieldIndex (&sv_pr_state, "maxs");
sv_fields.size = ED_GetFieldIndex (&sv_pr_state, "size");
sv_fields.touch = ED_GetFieldIndex (&sv_pr_state, "touch");
sv_fields.use = ED_GetFieldIndex (&sv_pr_state, "use");
sv_fields.think = ED_GetFieldIndex (&sv_pr_state, "think");
sv_fields.blocked = ED_GetFieldIndex (&sv_pr_state, "blocked");
sv_fields.nextthink = ED_GetFieldIndex (&sv_pr_state, "nextthink");
sv_fields.groundentity = ED_GetFieldIndex (&sv_pr_state, "groundentity");
sv_fields.health = ED_GetFieldIndex (&sv_pr_state, "health");
sv_fields.frags = ED_GetFieldIndex (&sv_pr_state, "frags");
sv_fields.weapon = ED_GetFieldIndex (&sv_pr_state, "weapon");
sv_fields.weaponmodel = ED_GetFieldIndex (&sv_pr_state, "weaponmodel");
sv_fields.weaponframe = ED_GetFieldIndex (&sv_pr_state, "weaponframe");
sv_fields.currentammo = ED_GetFieldIndex (&sv_pr_state, "currentammo");
sv_fields.ammo_shells = ED_GetFieldIndex (&sv_pr_state, "ammo_shells");
sv_fields.ammo_nails = ED_GetFieldIndex (&sv_pr_state, "ammo_nails");
sv_fields.ammo_rockets = ED_GetFieldIndex (&sv_pr_state, "ammo_rockets");
sv_fields.ammo_cells = ED_GetFieldIndex (&sv_pr_state, "ammo_cells");
sv_fields.items = ED_GetFieldIndex (&sv_pr_state, "items");
sv_fields.items2 = ED_GetFieldIndex (&sv_pr_state, "items2");
sv_fields.takedamage = ED_GetFieldIndex (&sv_pr_state, "takedamage");
sv_fields.chain = ED_GetFieldIndex (&sv_pr_state, "chain");
sv_fields.deadflag = ED_GetFieldIndex (&sv_pr_state, "deadflag");
sv_fields.view_ofs = ED_GetFieldIndex (&sv_pr_state, "view_ofs");
sv_fields.button0 = ED_GetFieldIndex (&sv_pr_state, "button0");
sv_fields.button1 = ED_GetFieldIndex (&sv_pr_state, "button1");
sv_fields.button2 = ED_GetFieldIndex (&sv_pr_state, "button2");
sv_fields.impulse = ED_GetFieldIndex (&sv_pr_state, "impulse");
sv_fields.fixangle = ED_GetFieldIndex (&sv_pr_state, "fixangle");
sv_fields.v_angle = ED_GetFieldIndex (&sv_pr_state, "v_angle");
sv_fields.idealpitch = ED_GetFieldIndex (&sv_pr_state, "idealpitch");
sv_fields.pitch_speed = ED_GetFieldIndex (&sv_pr_state, "pitch_speed");
sv_fields.netname = ED_GetFieldIndex (&sv_pr_state, "netname");
sv_fields.enemy = ED_GetFieldIndex (&sv_pr_state, "enemy");
sv_fields.flags = ED_GetFieldIndex (&sv_pr_state, "flags");
sv_fields.colormap = ED_GetFieldIndex (&sv_pr_state, "colormap");
sv_fields.team = ED_GetFieldIndex (&sv_pr_state, "team");
sv_fields.max_health = ED_GetFieldIndex (&sv_pr_state, "max_health");
sv_fields.teleport_time = ED_GetFieldIndex (&sv_pr_state, "teleport_time");
sv_fields.armortype = ED_GetFieldIndex (&sv_pr_state, "armortype");
sv_fields.armorvalue = ED_GetFieldIndex (&sv_pr_state, "armorvalue");
sv_fields.waterlevel = ED_GetFieldIndex (&sv_pr_state, "waterlevel");
sv_fields.watertype = ED_GetFieldIndex (&sv_pr_state, "watertype");
sv_fields.ideal_yaw = ED_GetFieldIndex (&sv_pr_state, "ideal_yaw");
sv_fields.yaw_speed = ED_GetFieldIndex (&sv_pr_state, "yaw_speed");
sv_fields.aiment = ED_GetFieldIndex (&sv_pr_state, "aiment");
sv_fields.goalentity = ED_GetFieldIndex (&sv_pr_state, "goalentity");
sv_fields.spawnflags = ED_GetFieldIndex (&sv_pr_state, "spawnflags");
sv_fields.target = ED_GetFieldIndex (&sv_pr_state, "target");
sv_fields.targetname = ED_GetFieldIndex (&sv_pr_state, "targetname");
sv_fields.dmg_take = ED_GetFieldIndex (&sv_pr_state, "dmg_take");
sv_fields.dmg_save = ED_GetFieldIndex (&sv_pr_state, "dmg_save");
sv_fields.dmg_inflictor = ED_GetFieldIndex (&sv_pr_state, "dmg_inflictor");
sv_fields.owner = ED_GetFieldIndex (&sv_pr_state, "owner");
sv_fields.movedir = ED_GetFieldIndex (&sv_pr_state, "movedir");
sv_fields.message = ED_GetFieldIndex (&sv_pr_state, "message");
sv_fields.sounds = ED_GetFieldIndex (&sv_pr_state, "sounds");
sv_fields.noise = ED_GetFieldIndex (&sv_pr_state, "noise");
sv_fields.noise1 = ED_GetFieldIndex (&sv_pr_state, "noise1");
sv_fields.noise2 = ED_GetFieldIndex (&sv_pr_state, "noise2");
sv_fields.noise3 = ED_GetFieldIndex (&sv_pr_state, "noise3");
sv_fields.dmg = ED_GetFieldIndex (&sv_pr_state, "dmg");
sv_fields.dmgtime = ED_GetFieldIndex (&sv_pr_state, "dmgtime");
sv_fields.air_finished = ED_GetFieldIndex (&sv_pr_state, "air_finished");
sv_fields.pain_finished = ED_GetFieldIndex (&sv_pr_state, "pain_finished");
sv_fields.radsuit_finished = ED_GetFieldIndex (&sv_pr_state, "radsuit_finished");
sv_fields.speed = ED_GetFieldIndex (&sv_pr_state, "speed");
}

void
SV_Progs_Init (void)
{
	sv_pr_state.edicts = &sv.edicts;
	sv_pr_state.num_edicts = &sv.num_edicts;
	sv_pr_state.time = &sv.time;
	sv_pr_state.reserved_edicts = &svs.maxclients;
	sv_pr_state.unlink = SV_UnlinkEdict;

	Cmd_AddCommand ("edict", ED_PrintEdict_f,
					"Report information on a given edict in the game. (edict (edict number))");
	Cmd_AddCommand ("edicts", ED_PrintEdicts_f,
					"Display information on all edicts in the game.");
	Cmd_AddCommand ("edictcount", ED_Count_f,
					"Display summary information on the edicts in the game.");
	Cmd_AddCommand ("profile", PR_Profile_f,
					"FIXME: Report information about QuakeC Stuff (???) No Description");
}

void
SV_Progs_Init_Cvars (void)
{
	r_skyname = Cvar_Get ("r_skyname", "", CVAR_SERVERINFO, "name of skybox");
	sv_progs = Cvar_Get ("sv_progs", "progs.dat", CVAR_ROM,
						 "Allows selectable game progs if you have several "
						 "of them in the gamedir");
	pr_checkextentions = Cvar_Get ("sv_progs", "1", CVAR_ROM,
								   "indicate the presence of the "
								   "checkextentions qc function");

	nomonsters = Cvar_Get ("nomonsters", "0", CVAR_NONE, "No Description");
	gamecfg = Cvar_Get ("gamecfg", "0", CVAR_NONE, "No Description");
	scratch1 = Cvar_Get ("scratch1", "0", CVAR_NONE, "No Description");
	scratch2 = Cvar_Get ("scratch2", "0", CVAR_NONE, "No Description");
	scratch3 = Cvar_Get ("scratch3", "0", CVAR_NONE, "No Description");
	scratch4 = Cvar_Get ("scratch4", "0", CVAR_NONE, "No Description");
	savedgamecfg = Cvar_Get ("savedgamecfg", "0", CVAR_ARCHIVE, "No Description");
	saved1 = Cvar_Get ("saved1", "0", CVAR_ARCHIVE, "No Description");
	saved2 = Cvar_Get ("saved2", "0", CVAR_ARCHIVE, "No Description");
	saved3 = Cvar_Get ("saved3", "0", CVAR_ARCHIVE, "No Description");
	saved4 = Cvar_Get ("saved4", "0", CVAR_ARCHIVE, "No Description");
}
