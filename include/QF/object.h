/*
	object.h

	Provides a primitive object framework to back objects
	in higher level languages of QF so that they can share
	objects.  For example, Ruamoko and GIB would be able to
	pass String objects to each other, even if the higher
	-level implementations of String in each language differ.

	Copyright (C) 2003 Brian Koropoff

	Author: Brian Koropoff
	Date: November 28, 2003

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

#ifndef __object_h
#define __object_h

#include "QF/qtypes.h"

#ifdef __GNUC__
#define superInit(cl, obj, args...) (cl##_class->parent->init ((obj) , ## args))
#define new(cl, args...) ((void *) (cl##_class->abstract ? NULL : cl##_class->init (Object_Create(cl##_class, false), ## args)))
#define newFloat(cl, args...) ((void *) (cl##_class->abstract ? NULL : cl##_class->init (Object_Create(cl##_class, true) , ## args)))
#define methodCall(obj, m, args...) ((obj)->m(obj , ## args))
#define methodDecl(type, name, args...) (* name) (struct type##_s *self , ## args)
#else
#define superInit(cl, obj, ...) (cl##_class->parent->init ((obj), ##__VA_ARGS__))
#define new(cl, ...) ((void *) (cl##_class->abstract ? NULL : cl##_class->init (Object_Create(cl##_class, false) , ##__VA_ARGS__)))
#define newFloat(cl, ...) ((void *) (cl##_class->abstract ? NULL : cl##_class->init (Object_Create(cl##_class, true) , ##__VA_ARGS__)))
#define methodCall(obj, m, ...) ((obj)->m(obj, ##__VA_ARGS__))
#define methodDecl(type, name, ...) (* name) (struct type##_s *self, ##__VA_ARGS__)
#endif

#define classObj(name) name##_class
#define classDecl(name,extends,def) typedef struct name##_s {struct extends##_s base; def} name; extern Class * classObj(name); void __class_##name##_init (void)
#define classInitFunc(name) Class * classObj(name); void __class_##name##_init (void)
#define classInit(name) __class_##name##_init()
#define retain(obj) (Object_Retain((Object *)obj))
#define release(obj) (Object_Release((Object *)obj))

#define instanceOf(obj, cl) (Object_InstaceOf((Object *)obj, cl##_class))

struct Object_s;
struct Class_s;
struct List_s;

typedef void (*ReplyHandler_t) (struct Object_s *retValue);

typedef struct Object_s {
	struct Class_s *cl;
	int refs;
	qboolean junked;
	struct Object_s *next;
	struct String_s * methodDecl(Object, toString);
	void methodDecl(Object, message, const char *name, struct List_s *args, struct Object_s *sender, ReplyHandler_t *reply);
	void *data;

} Object;
extern struct Class_s * Object_class;
#define OBJECT(o) ((Object *)(o))


typedef Object *(*Object_Init_t) (Object *obj, ...);
typedef void (*Object_Deinit_t) (Object *obj);

classDecl (Class, Object,
	qboolean abstract;
	unsigned int size;
	const char *name;
	struct Class_s *parent;
	Object_Init_t init;
	Object_Deinit_t deinit;
);
#define CLASS(o) ((Class *)(o))

Object *Object_Create (Class *cl, qboolean floating);
void Object_Delete (Object *obj);
Object *Object_Retain (Object *obj);
Object *Object_Release (Object *obj);
qboolean Object_InstanceOf (Object *obj, Class *cl);
void Object_Init (void);
void Object_Garbage_Collect (void);

#include "QF/classes/List.h"
#include "QF/classes/String.h"

#endif