#pragma once

#include "base.h"

typedef enum
{
#define X(identifier, code, representation) token_tag_##identifier = code,
	#include "code_tokens.inc"
#undef X
} token_tag;

constexpr utf8 token_tag_representations[][40] =
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

typedef enum
{
#define X(identifier, body, syntax) node_tag_##identifier,
	#include "code_nodes.inc"
#undef X
} node_tag;

constexpr utf8 node_tag_representations[][40] =
{
#define X(identifier, body, syntax) [node_tag_##identifier] = #identifier,
	#include "code_nodes.inc"
#undef X
};

constexpr utf8 node_syntaxes[][40] =
{
#define X(identifier, body, syntax) [node_tag_##identifier] = syntax,
	#include "code_nodes.inc"
#undef X
};

typedef struct node node;

#define X(identifier, body, syntax) typedef struct identifier##_node body identifier##_node;
	#include "code_nodes.inc"
#undef X

struct node
{
	node_tag tag;
	union
	{
#define X(identifier, body, syntax) identifier##_node identifier;
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
