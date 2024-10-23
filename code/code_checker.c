#include "code_parser.h"

typedef enum
{
	symbol_type_value,
	symbol_type_tuple,
	symbol_type_procedure,
	symbol_types_count,
} symbol_type;

typedef struct
{
	utf8 identifier[32];
	uint identifier_size;
} symbol;

typedef struct
{
	symbol *symbols;
	uint    symbols_count;
} symbols_row;

typedef struct symbol_table symbol_table;

/* this symbol table is good for local symbols. for global symbols, a hash table
   is used.
   
   we use different structures because globals are expected to be abundant,
   unlike locals.

   the "global symbol table" is defined within the `program` structure. */
struct symbol_table
{
	symbol_table *parent;
	symbol_table *children;
	uint          children_count;

	symbol_row value_rows;
	symbol_row tuple_rows;
	symbol_row procedure_rows;
};

symbol *get_symbol(symbol_table *table)
{
	
}

#if 0 /* we dont' need this at the moment */

typedef enum : uintb
{
	basic_type_tuple,
	basic_type_uint8,
	basic_type_uint16,
	basic_type_uint32,
	basic_type_uint64,
	basic_type_sint8,
	basic_type_sint16,
	basic_type_sint32,
	basic_type_sint64,
	basic_type_real32,
	basic_type_real64,
} basic_type;

constexpr utf8 representations_of_basic_types[][8] =
{
	[basic_type_tuple]  = "tuple",
	[basic_type_uint8]  = "uint8",
	[basic_type_uint16] = "uint16",
	[basic_type_uint32] = "uint32",
	[basic_type_uint64] = "uint64",
	[basic_type_sint8]  = "sint8",
	[basic_type_sint16] = "sint16",
	[basic_type_sint32] = "sint32",
	[basic_type_sint64] = "sint64",
	[basic_type_real32] = "real32",
	[basic_type_real64] = "real64",
};

#endif
