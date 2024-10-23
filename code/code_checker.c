#include "code_parser.h"

typedef enum : uintb
{
	symbol_type_procedure,
	symbol_type_tuple,
	symbol_type_value,
} symbol_type;

typedef struct
{
	utf8 identifier[32]; /* TODO: allow larger identifiers */
	uint identifier_size;
	node *node;
} symbol;

typedef struct
{
	symbol *symbols;
	uint    symbols_count;
} symbol_table_row;

typedef struct symbol_table symbol_table;

struct symbol_table
{
	symbol_table_row *rows;
	uint              rows_count;
};

uint hash_for_symbol_table(const utf8 *identifier, uint identifier_size, symbol_table *table)
{
	uint hash = 0;
	for (uint i = 0; i < identifier_size; ++i) hash += identifier[i];
	hash %= table->rows_count;
	return hash;
}

symbol *get_symbol(const identifier_node *identifier, symbol_table *table)
{
	uint hash = hash_for_symbol_table(identifier->runes, identifier->runes_count, table);
	symbol_table_row *row = &table->rows[hash];
	for (uint i = 0; i < row->symbols_count; ++i)
	{
		symbol *symbol = &row->symbols[i];
		if (!compare_sized_text(symbol->identifier, identifier->runes, identifier->runes_count))
		{
			return symbol;
		}
	}
	return 0;
}

void check_node(node *node)
{
	if (is_symbol_usage)
	{
		symbol *symbol = get_symbol(identifier, &current_scope->symbol_table);
		if (symbol)
		{
			
		}
		else
		{
			push_undefined_symbol(node->data->identifier, &parser->current_scope->symbol_table);
		}
	}
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
