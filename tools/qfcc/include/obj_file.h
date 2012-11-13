/*
	obj_file.h

	object file support

	Copyright (C) 2002 Bill Currie <bill@taniwha.org>

	Author: Bill Currie <bill@taniwha.org>
	Date: 2002/6/16

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

#ifndef __obj_file_h
#define __obj_file_h

/** \defgroup qfcc_qfo Object file functions
	\ingroup qfcc
*/
//@{

#include "QF/pr_comp.h"
#include "QF/pr_debug.h"
#include "QF/quakeio.h"

/** Identifier string for qfo object files (includes terminating nul)

	\hideinitializer
*/
#define QFO			"QFO"
/** QFO object file format version (MMmmmRRR 0.001.006 (hex))

	\hideinitializer
*/
#define QFO_VERSION 0x00001006

/** Header block of QFO object files. The sections of the object file
	come immediately after the header, and are always in the order given by
	the struct.

	All indices to records are 0-based from the beginning of the relevant
	section.
*/
typedef struct qfo_header_s {
	int8_t      qfo[4];			///< identifier string (includes nul) (#QFO)
	pr_int_t    version;		///< QFO format version (#QFO_VERSION)
	pr_int_t    num_spaces;
	pr_int_t    num_relocs;		///< number of relocation records
	pr_int_t    num_defs;		///< number of def records
	pr_int_t    num_funcs;		///< number of function records
	pr_int_t    num_lines;		///< number of line records
	pr_int_t    num_loose_relocs;	///< number of loose relocation records
								///< included in num_relocs
} qfo_header_t;

typedef enum qfos_type_e {
	qfos_null,					///< null space. no data or defs. first in qfo
	qfos_code,					///< progs code. dstatement_t data
	qfos_data,					///< progs data. pr_type_t data
	qfos_string,				///< strings. char data
	qfos_entity,				///< entity field defs. no data
	qfos_type,					///< type encodings
} qfos_type_t;

/**	Representation of a space in the object file.
*/
typedef struct qfo_space_s {
	pr_int_t    type;			///< code, string, data, entity...
	pr_int_t    defs;			///< index of first def
	pr_int_t    num_defs;		///< zero for code or string spaces
	pr_int_t    data;			///< byte offset in qfo
	pr_int_t    data_size;		///< in elements. zero for entity spaces
	pr_int_t    id;
	pr_int_t    reserved[2];
} qfo_space_t;

/** Representation of a def in the object file.
*/
typedef struct qfo_def_s {
	pointer_t   type;			///< offset in type space
	string_t    name;			///< def name
	pointer_t   offset;			///< def offset (address)

	pr_int_t    relocs;			///< index of first reloc record
	pr_int_t    num_relocs;		///< number of reloc records

	pr_uint_t   flags;			///< \ref qfcc_qfo_QFOD "QFOD flags"

	string_t    file;			///< source file name
	pr_int_t    line;			///< source line number
} qfo_def_t;
//@}

/** \defgroup qfcc_qfo_QFOD QFOD flags
	\ingroup qfcc_qfo
*/
//@{

/** The def has been initialized.

	\hideinitializer
*/
#define QFOD_INITIALIZED	(1u<<0)

/** The def is a shared constant and must not be modified.

	\hideinitializer
*/
#define QFOD_CONSTANT		(1u<<1)

/** The def offset is an absolute address.

	\hideinitializer
*/
#define QFOD_ABSOLUTE		(1u<<2)

/** The def is visible to other object files.

	\hideinitializer
*/
#define QFOD_GLOBAL			(1u<<3)

/** The def is undefined and need to be supplied by another object file.

	\hideinitializer
*/
#define QFOD_EXTERNAL		(1u<<4)

/** The def is local to a function and is not visible to other functions or
	object files.

	\hideinitializer
*/
#define QFOD_LOCAL			(1u<<5)

/** The def is a system def and needs to be allocated before other defs

	\hideinitializer
*/
#define QFOD_SYSTEM			(1u<<6)

/** The def does not need to be saved when saving game state.

	\hideinitializer
*/
#define QFOD_NOSAVE			(1u<<7)
//@}

/** \addtogroup qfcc_qfo
*/
//@{

/** Representation of a function in the object file.
*/
typedef struct qfo_func_s {
	string_t    name;			///< function name
	pointer_t   type;			///< function type (in type data space)
	string_t    file;			///< source file name
	pr_int_t    line;			///< source line number

	/** \name Function code location.
		If #code is negative, then the function is a VM builtin function.
		If #code is 0, then the function is a VM builtin function and the
		VM resolves the function number using the function name.
		If #code is positive, then the function is in progs and #code is
		the first statement of the function.
	*/
	pr_int_t    code;

	pr_int_t    def;			///< def that references this function. Index
								///< to ::qfo_def_t. The data word pointed to
								///< by the def stores the index of this
								///< function.

	pr_int_t    locals_space;	///< space holding the function's local data

	pr_int_t    line_info;		///< Index to first ::pr_lineno_t line record.
								///< Zero if there are no records.

	/** \name Function relocation records.
		XXX not sure how these work
	*/
	//@{
	pr_int_t    relocs;			///< Index to first ::qfo_reloc_t reloc record.
	pr_int_t    num_relocs;		///< Number of reloc records.
	//@}
	pr_int_t    reserved[2];
} qfo_func_t;

/** Evil source of many headaches. The whole reason I've started writing this
	documentation.
	relocs are always in the order:
		referenced relocs
		unreferenced relocs

	For \c ref_op_* relocation types, \c ofs is the code section address of the
	statement that needs to be adjusted.

	For \c rel_def_* relocation types,
	\c ofs refers to the data section address of the word that needs to be
	adjusted.

	For \c ref_*_def(_ofs) relocation types, \c def is the index of the
	referenced def.

	For \c ref_*_op relocation types, \c def is the address of
	the referenced statement.

	For \c ref_*_string relocation types, \c def is
	always 0.

	For \c ref_*_field(_ofs) relocation types, \c def is the index of
	the referenced field def.
*/
typedef struct qfo_reloc_s {
	pr_int_t    space;			///< index of space holding data to be adjusted
	pr_int_t    offset;			///< offset of the relocation
	pr_int_t    type;			///< type of the relocation (::reloc_type)
	pr_int_t    target;			///< def/func/etc this relocation is for
} qfo_reloc_t;

/**	In-memory representation of a QFO space
*/
typedef struct qfo_mspace_s {
	qfos_type_t type;
	qfo_def_t  *defs;
	int         num_defs;
	union {
		dstatement_t *code;
		pr_type_t  *data;
		char       *strings;
	}           d;
	int         data_size;
	int         id;
} qfo_mspace_t;

/** In-memory representation of a QFO object file.
*/
typedef struct qfo_s {
	void       *data;			///< data buffer holding qfo file when read
	qfo_mspace_t *spaces;
	int         num_spaces;
	qfo_reloc_t *relocs;
	int         num_relocs;			// includes num_loose_relocs
	qfo_def_t  *defs;
	int         num_defs;
	qfo_func_t *funcs;
	int         num_funcs;
	pr_lineno_t *lines;
	int         num_lines;
	int         num_loose_relocs;	// included in num_relocs
} qfo_t;

enum {
	qfo_null_space,
	qfo_strings_space,
	qfo_code_space,
	qfo_near_data_space,
	qfo_far_data_space,
	qfo_entity_space,
	qfo_type_space,

	qfo_num_spaces
};
//@}

/** \defgroup qfcc_qfo_data_access QFO Data Acess
	\ingroup qfcc_qfo
	Macros for accessing data in the QFO address space
*/
//@{

/** \internal
	\param q pointer to ::qfo_t struct
	\param s space index
	\param t typename prefix (see pr_type_u)
	\param o offset into object file data space
	\return lvalue of the appropriate type

	\hideinitializer
*/
#define QFO_var(q, s, t, o)	((q)->spaces[s].d.data[o].t##_var)

/** Access a float variable in the object file. Can be assigned to.

	\par QC type:
		\c float
	\param q pointer to ::qfo_t struct
	\param s space index
	\param o offset into object file data space
	\return float lvalue

	\hideinitializer
*/
#define	QFO_FLOAT(q, s, o)		QFO_var (q, s, float, o)

/** Access a integer variable in the object file. Can be assigned to.

	\par QC type:
		\c integer
	\param q pointer to ::qfo_t struct
	\param s space index
	\param o offset into object file data space
	\return int lvalue

	\hideinitializer
*/
#define	QFO_INT(q, s, o)		QFO_var (q, s, integer, o)

/** Access a vector variable in the object file. Can be assigned to.

	\par QC type:
		\c vector
	\param q pointer to ::qfo_t struct
	\param s space index
	\param o offset into object file data space
	\return vec3_t lvalue

	\hideinitializer
*/
#define	QFO_VECTOR(q, s, o)	QFO_var (q, s, vector, o)

/** Access a string index variable in the object file. Can be assigned to.

	\par QC type:
		\c string
	\param q pointer to ::qfo_t struct
	\param s space index
	\param o offset into object file data space
	\return string_t lvalue

	\hideinitializer
*/
#define	QFO_STRING(q, s, o)	QFO_var (q, s, string, o)

/** Retrieve a string from the object file, converting it to a C string.

	\param q pointer to ::qfo_t struct
	\param s space index
	\param s offset into object file string space
	\return (char *)

	\hideinitializer
*/
#define QFO_GETSTR(q, s)	((q)->spaces[qfo_strings_space].d.strings + (s))

#define QFO_TYPE(q, t)		((qfot_type_t *) (char *) \
							 ((q)->spaces[qfo_type_space].d.data + (t)))

/** Retrieve a type string from the object file, converting it to a C string.

	\param q pointer to ::qfo_t struct
	\param t offset to type encoding
	\return (char *)

	\note Assumes standard space order.
	\hideinitializer
*/
#define QFO_TYPESTR(q, t)	QFO_GSTRING (q, qfo_type_space, (t) + 2)

#define QFO_TYPEMETA(q, t)	QFO_INT (q, qfo_type_space, (t) + 0)
#define QFO_TYPETYPE(q, t)	QFO_INT (q, qfo_type_space, (t) + 3)

#define QFO_STATEMENT(q, s) ((q)->spaces[qfo_code_space].d.code + (s))

/** Access a string global, converting it to a C string.

	\param q pointer to ::qfo_t struct
	\param s space index
	\param o offset into object file data space
	\return (char *)

	\hideinitializer
*/
#define QFO_GSTRING(q, s, o)	(QFO_GETSTR (q, QFO_STRING (q, s, o)))

/** Access a function variable in the object file. Can be assigned to.

	\par QC type:
		\c void ()
	\param q pointer to ::qfo_t struct
	\param s space index
	\param o offset into object file data space
	\return func_t lvalue

	\hideinitializer
*/
#define	QFO_FUNCTION(q, s, o)	QFO_var (q, s, func, o)

/** Access a pointer variable in the object file. Can be assigned to.

	\par QC type:
		\c void []
	\param q pointer to ::qfo_t struct
	\param s space index
	\param t C type of the structure
	\param o offset into object file data space
	\return pointer_t lvalue

	\hideinitializer
*/
#define QFO_POINTER(q, s, t, o)	((t *)(char *)&QFO_var (q, s, integer, o))

/** Access a structure variable in the object file. Can be assigned to.

	\par QC type:
		\c void [](?)
	\param q pointer to ::qfo_t struct
	\param s space index
	\param t C type of the structure
	\param o offset into object file data space
	\return structure lvalue. use & to make a pointer of the appropriate type.

	\hideinitializer
*/
#define QFO_STRUCT(q, s, t, o)	(*QFO_POINTER (q, s, t, o))

//@}

/** \addtogroup qfcc_qfo
*/
//@{

struct pr_info_s;

/** Convert ::pr_info_t structure to ::qfo_t.
	\param pr pointer to ::pr_info_t struct
	\return pointer to new ::qfo_t struct, or 0 on error.
*/
qfo_t *qfo_from_progs (struct pr_info_s *pr);

/** Write a ::qfo_t struct to the named file.
	\param qfo pointer to ::qfo_t struct to write
	\param filename name of the file to write
	\return 0 for success, -1 for error.
*/
int qfo_write (qfo_t *qfo, const char *filename);

/** Read a ::qfo_t strcut from a ::QFile stream.
	\param file ::QFile stream to read from
	\return pointer to new ::qfo_t struct, or 0 on error.
*/
qfo_t *qfo_read (QFile *file);

/** Wrapper around qfo_read() to allow reading from a named file.
	\param filename name of the file to read
	\return pointer to new ::qfo_t struct, or 0 on error.
*/
qfo_t *qfo_open (const char *filename);

dprograms_t *qfo_to_progs (qfo_t *qfo, int *size);
pr_debug_header_t *qfo_to_sym (qfo_t *qfo, int *size);

/** Create a new ::qfo_t struct
	\return pointer to new ::qfo_t struct, or 0 on error.
*/
qfo_t *qfo_new (void);

/** Delete a ::qfo_t struct, as well as any substructure data.
	\param qfo ::qfo_t struct to delete
*/
void qfo_delete (qfo_t *qfo);

//@}

#endif//__obj_file_h
