/*
	#FILENAME#

	#DESCRIPTION#

	Copyright (C) 2004 #AUTHOR#

	Author: #AUTHOR#
	Date: #DATE#

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

#include <stdio.h>
#include <stdlib.h>

#include "QF/cmd.h"
#include "QF/console.h"
#include "QF/hash.h"
#include "QF/sys.h"

#include "connection.h"

static hashtab_t *connections;

static unsigned long
connection_get_hash (void *_c, void *unused)
{
	connection_t *c = (connection_t *) _c;
	unsigned long hash;

	hash = Hash_Buffer (c->address.ip, sizeof (c->address.ip));
	hash ^= c->address.port;
	return hash;
}

static int
connection_compare (void *_c1, void *_c2, void *unused)
{
	connection_t *c1 = (connection_t *) _c1;
	connection_t *c2 = (connection_t *) _c2;

	return NET_CompareAdr (c1->address, c2->address);
}

void
Connection_Init (void)
{
	connections = Hash_NewTable (1023, 0, 0, 0);
	Hash_SetHashCompare (connections, connection_get_hash, connection_compare);
}

connection_t *
Connection_Add (netadr_t *address, void *object,
				void (*handler)(connection_t *, void *))
{
	connection_t *con;

	con = malloc (sizeof (connection_t));
	con->address = *address;
	con->object = object;
	con->handler = handler;
	if (Hash_FindElement (connections, con))
		Sys_Error ("duplicate connection");
	Hash_AddElement (connections, con);
	return con;
}

void
Connection_Del (connection_t *con)
{
	Hash_DelElement (connections, con);
}

connection_t *
Connection_Find (netadr_t *address)
{
	connection_t con;

	con.address = *address;
	return Hash_FindElement (connections, &con);
}