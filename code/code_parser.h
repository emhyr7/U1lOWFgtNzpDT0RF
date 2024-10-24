#if !defined(CODE_PARSER_H)
#define CODE_PARSER_H

#include "code.h"

typedef enum : uintb
{
#define X(identifier, code, representation) token_tag_##identifier = code,
	#include "code_tokens.inc"
#undef X
} token_tag;

constexpr utf8 token_tags_representations[][40] =
{
#define X(identifier, code, representation) [token_tag_##identifier] = representation,
	#include "code_tokens.inc"
#undef X
};

typedef struct
{
	token_tag tag;
	uint beginning;
	uint ending;
	uint row;
	uint column;
} token;

typedef enum : uintb 
{
#define X(type, identifier, body, syntax) node_tag_##identifier,
	#include "code_nodes.inc"
#undef X
} node_tag;

constexpr utf8 node_tags_representations[][40] =
{
#define X(type, identifier, body, syntax) [node_tag_##identifier] = #identifier,
	#include "code_nodes.inc"
#undef X
};

constexpr utf8 node_syntaxes[][40] =
{
#define X(type, identifier, body, syntax) [node_tag_##identifier] = syntax,
	#include "code_nodes.inc"
#undef X
};

typedef struct node node;

typedef enum
{
	pragma_undefined,
	pragma_fp64,
} pragma;

#define X(type, identifier, body, syntax) typedef struct identifier##_node body identifier##_node;
	#include "code_nodes.inc"
#undef X

typedef enum : uintb
{
	literal_type_none,
	literal_type_undefined,
	literal_type_digital,
	literal_type_decimal,
	literal_type_array,
	literal_type_address,
	literal_type_tuple,
} type_type;

typedef struct type;

typedef struct
{
	type *type;
	uint count;
} array_type;

typedef struct
{
	type *types;
	uint types_count;
} tuple_type;

table_value *get_from_table(const void *key, uint key_size, table *table)
{
	table_value *value = 0;
	for (uint i = 0; i < table->entries_count; ++i)
	{
		table_key *key = &table->keys[i];
		if (!compare(key, key->data, MINIMUM(key->size, key_size)))
		{
			value = &table->values[i];
			break;
		}
	}
	return value;
}

mapping_table_entry *get_from_mapping_table(const void *key, uint key_size, mapping_table *table)
{
	uint entry_index = (uint)get_from_table(key, key_size, &table->mappings);
	mapping_table_entry *entry = &table->entries[entry_index];
	return entry;
}

type *get_type(const char *name, uint name_size, mapping_table *table)
{
	mapping_table_entry *entry = get_from_mapping_table(table);
	type *type = (type *)entry->data;
	return type;
}

type *get_node_type(node *node)
{
	type *type = 0;
	if (is_unary)
	{
		if (!node->type) node->type = get_node_type(node->data->unary.node);
		type = node->type;
	}
	else if (is_binary)
	{
	}
	else if (is_ternary)
	{
	}
	else switch (node->tag)
	{
	case node_tag_digital:
		return basic_types.digital;
	case node_tag_decimal:
		return basic_types.decimal;

	case node_tag_text:
		/* array */
		break;

	case node_tag_identifier:
	case node_tag_resolution:
		/* path */
		break;
	
	case node_tag_subexpression:
		/* tuple */
		break;
	}
	return type;
}

struct type
{
	type_type type;
	union
	{
		array_type array;
		tuple_type tuple;
	} data[];
};

typedef struct
{
} type_mapping;

typedef struct
{
	
} type_table;

struct node
{
	node_tag tag;

	type *type;

	union
	{
#define X(type, identifier, body, syntax) identifier##_node identifier;
	#include "code_nodes.inc"
#undef X
	} data[];
};

typedef struct
{
	scope_node globe;
} program;

typedef struct
{
	regional_allocator allocator;

	const utf8 *source_path;
	uint        source_size;
	utf8       *source;

	uint position;
	uint row;
	uint column;

	utf32 rune;
	uintb increment;

	bit parsing_finished : 1;

	landing *failure_landing;

	token       token;
	program    *program;
	scope_node *current_scope;
} parser;

void parser_parse(const utf8 *source_path, program *program, parser *parser);

#endif
