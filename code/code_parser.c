#include "code_parser.h"

static void report_source_v(severity severity, const utf8 *source_path, const utf8 *source, uint beginning, uint ending, uint row, uint column, const utf8 *message, vargs vargs)
{
	fprintf(stderr, "[%s] %s(%u|%u:%u): ", severities_representations[severity], source_path, beginning, row, column);
	vfprintf(stderr, message, vargs);
	fputc('\n', stderr);

	if (beginning == ending) return;

	/* begin from the beginning of the line */
	const utf8 *caret = source + beginning - column + 1;

	/* print up to the beginning */
	fprintf(stderr, "\t%u | ", row++);
	while (caret != source + beginning) fputc(*caret++, stderr);

	/* print the source */
	const utf8 severity_colors[][8] =
	{
		[severity_verbose] = "\x1b[1;34m",
		[severity_comment] = "\x1b[1;32m",
		[severity_caution] = "\x1b[1;33m",
		[severity_failure] = "\x1b[1;31m",
	};
	fprintf(stderr, "%s", severity_colors[severity]);
	while (caret != source + ending)
	{
		if (*caret == '\n')
		{
			fprintf(stderr, "\t%u | ", row);
			++row;
		}
		fputc(*caret++, stderr);
	}
	fprintf(stderr, "\x1b[0m");

	/* print to the end of the line, or end of the source */
	while (*caret != '\n' && *caret != '\0') fputc(*caret++, stderr);
	printf("\n\n", stderr);

	fflush(stderr);
}

static inline void report_source(severity severity, const utf8 *source_path, const utf8 *source, uint beginning, uint ending, uint row, uint column, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); report_source_v(severity, source_path, source, beginning, ending, row, column, message, vargs); END_VARGS(vargs); }

static inline void report_source_verbose(const utf8 *source_path, const utf8 *source, uint beginning, uint ending, uint row, uint column, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); report_source_v(severity_verbose, source_path, source, beginning, ending, row, column, message, vargs); END_VARGS(vargs); }
static inline void report_source_comment(const utf8 *source_path, const utf8 *source, uint beginning, uint ending, uint row, uint column, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); report_source_v(severity_comment, source_path, source, beginning, ending, row, column, message, vargs); END_VARGS(vargs); }
static inline void report_source_caution(const utf8 *source_path, const utf8 *source, uint beginning, uint ending, uint row, uint column, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); report_source_v(severity_caution, source_path, source, beginning, ending, row, column, message, vargs); END_VARGS(vargs); }
static inline void report_source_failure(const utf8 *source_path, const utf8 *source, uint beginning, uint ending, uint row, uint column, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); report_source_v(severity_failure, source_path, source, beginning, ending, row, column, message, vargs); END_VARGS(vargs); }

static inline void parser_report_v(severity severity, parser *parser, const utf8 *message, vargs vargs) { report_source_v(severity, parser->source_path, parser->source, parser->token.beginning, parser->token.ending, parser->token.row, parser->token.column, message, vargs); }
static inline void parser_report(severity severity, parser *parser, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); parser_report_v(severity, parser, message, vargs); END_VARGS(vargs); }

static inline void parser_report_verbose(parser *parser, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); parser_report_v(severity_verbose, parser, message, vargs); END_VARGS(vargs); }
static inline void parser_report_comment(parser *parser, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); parser_report_v(severity_comment, parser, message, vargs); END_VARGS(vargs); }
static inline void parser_report_caution(parser *parser, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); parser_report_v(severity_caution, parser, message, vargs); END_VARGS(vargs); }
static inline void parser_report_failure(parser *parser, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); parser_report_v(severity_failure, parser, message, vargs); END_VARGS(vargs); }

static uintb parser_peek(utf32 *rune, parser *parser)
{
	sintb increment;
	uint peek_position = parser->position + parser->increment;
	if (peek_position >= parser->source_size)
	{
		*rune = '\x3';
		increment = 0;
	}
	/* WARN: this can segfault */
	else
	{
		increment = decode_utf8(rune, parser->source + peek_position);
		if (increment < 0)
		{
			report_source_failure(parser->source_path, parser->source, parser->position, parser->position + -increment, parser->row, parser->column, "Unknown rune.");
			jump(*parser->failure_landing, 1);
		}
	}
	return increment;
}

static utf32 parser_advance(parser *parser)
{
	utf32 rune;
	uintb increment = parser_peek(&rune, parser);

	parser->position += parser->increment;
	if (parser->rune == '\n')
	{
		parser->row += 1;
		parser->column = 0;
	}
	parser->column += 1;
	parser->rune = rune;
	parser->increment = increment;
	return rune;
}

static inline bit is_whitespace(utf32 rune)
{
	return (rune >= '\t' && rune <= '\r') || (rune == ' ');
}

static inline bit is_letter(utf32 rune)
{
	return (rune >= 'A' && rune <= 'Z') || (rune >= 'a' && rune <= 'z');
}

static inline bit is_digit(utf32 rune)
{
	return rune >= '0' && rune <= '9';
}

static void parser_load(const utf8 *source_path, parser *parser)
{
	REPORT_VERBOSE("Loading source: %s\n", source_path);

	parser->source_path = source_path;
	file_handle source_file = open_file(parser->source_path);
	parser->source_size = get_size_of_file(source_file);
	parser->source = push(align_forward(parser->source_size + 1, sizeof(utf32)), universal_alignment, &parser->allocator);
	read_from_file(parser->source, parser->source_size, source_file);
	close_file(source_file);
	parser->source[parser->source_size] = '\3';

	parser->position  = 0;
	parser->row       = 0;
	parser->rune      = '\n';
	parser->increment = 0;
	parser_advance(parser);
}

static token_tag parser_get_token(parser *parser)
{
repeat:
	while (is_whitespace(parser->rune)) parser_advance(parser);

	parser->token.beginning = parser->position;
	parser->token.row       = parser->row;
	parser->token.column    = parser->column;

	utf32 peeked_rune;
	uintb peeked_increment = parser_peek(&peeked_rune, parser);

	switch (parser->rune)
	{
	case '\3':
		parser->token.tag = token_tag_etx;
		break;
	case '!':
		if (peeked_rune == '=')
		{
			parser->token.tag = token_tag_exclamation_mark_equal_sign;
			goto advance_twice;
		}
		else goto set_single;
	case '%':
		if (peeked_rune == '=')
		{
			parser->token.tag = token_tag_percent_sign_equal_sign;
			goto advance_twice;
		}
		else goto set_single;
	case '&':
		if (peeked_rune == '=')
		{
			parser->token.tag = token_tag_ampersand_equal_sign;
			goto advance_twice;
		}
		else if (peeked_rune == '&')
		{
			parser->token.tag = token_tag_ampersand_2;
			goto advance_twice;
		}
		else goto set_single;
	case '*':
		if (peeked_rune == '=')
		{
			parser->token.tag = token_tag_asterisk_equal_sign;
			goto advance_twice;
		}
		else goto set_single;
	case '+':
		if (peeked_rune == '=')
		{
			parser->token.tag = token_tag_plus_sign_equal_sign;
			goto advance_twice;
		}
		else goto set_single;
	case '-':
		if (peeked_rune == '=')
		{
			parser->token.tag = token_tag_minus_sign_equal_sign;
			goto advance_twice;
		}
		else if (peeked_rune == '>')
		{
			parser->token.tag = token_tag_minus_sign_greater_than_sign;
			goto advance_twice;
		}
		else if (peeked_rune == '-')
		{
			do parser_advance(parser);
			while (parser->rune != '\n');
			goto repeat;
		}
		else goto set_single;
	case '/':
		if (peeked_rune == '=')
		{
			parser->token.tag = token_tag_slash_equal_sign;
			goto advance_twice;
		}
		else goto set_single;
	case '<':
		if (peeked_rune == '<')
		{
			parser_advance(parser);
			peeked_increment = parser_peek(&peeked_rune, parser);
			if (peeked_rune == '=')
			{
				parser->token.tag = token_tag_less_than_sign_2_equal_sign;
			}
			else parser->token.tag = token_tag_less_than_sign_2;
			goto advance_twice;
		}
		else if (peeked_rune == '=')
		{
			parser->token.tag = token_tag_less_than_sign_equal_sign;
			goto advance_twice;
		}
		else goto set_single;
	case '=':
		if (peeked_rune == '=')
		{
			parser->token.tag = token_tag_equal_sign_2;
			goto advance_twice;
		}
		else goto set_single;
	case '>':
		if (peeked_rune == '>')
		{
			parser_advance(parser);
			peeked_increment = parser_peek(&peeked_rune, parser);
			if (peeked_rune == '=')
			{
				parser->token.tag = token_tag_greater_than_sign_2_equal_sign;
				goto advance_twice;
			}
			else parser->token.tag = token_tag_greater_than_sign_2;
			goto advance_twice;
		}
		else if (peeked_rune == '=')
		{
			parser->token.tag = token_tag_greater_than_sign_equal_sign;
			goto advance_twice;
		}
		else goto set_single;
	case '^':
		if (peeked_rune == '=')
		{
			parser->token.tag = token_tag_circumflex_accent_equal_sign;
			goto advance_twice;
		}
		else goto set_single;
	case '|':
		if (peeked_rune == '|')
		{
			parser->token.tag = token_tag_vertical_bar_2;
			goto advance_twice;
		}
		else if (peeked_rune == '=')
		{
			parser->token.tag = token_tag_vertical_bar_equal_sign;
			goto advance_twice;
		}
		else goto set_single;
	case '"':
		for (;;)
		{
			parser_advance(parser);
			if (parser->rune == '"') break;
			if (parser->rune == '\\')
			{
				switch (parser_advance(parser))
				{
				default:
					parser_advance(parser);
					break;
				}
			}
		}
		parser_advance(parser); /* skip the terminating `"` */
		parser->token.tag = token_tag_text;
		break;
	case '#':
	case '$':
	case '(':
	case ')':
	case ',':
	case '.':
	case ':':
	case ';':
	case '?':
	case '@':
	case '[':
	case '\\':
	case ']':
	case '`':
	case '{':
	case '}':
	case '~':
	set_single:
		parser->token.tag = (token_tag)parser->rune;
		goto advance_once;

	advance_twice:
		parser_advance(parser);
	advance_once:
		parser_advance(parser);
		break;

	default:
		if (is_letter(parser->rune) || parser->rune == '_')
		{
			do parser_advance(parser);
			while (is_letter(parser->rune) || parser->rune == '_' || parser->rune == '-' || is_digit(parser->rune));
			parser->token.tag = token_tag_identifier;
		}
		else if (is_digit(parser->rune))
		{
			parser->token.tag = token_tag_digital;
#if 0
			if (parser->rune == '0')
			{
				parser_advance(parser);
				switch (parser->rune)
				{
				case 'b': parser->token.tag = token_tag_binary; break;
				case 'x': parser->token.tag = token_tag_hexadecimal; break;
				default: break;
				}
			}
#endif

			while (is_digit(parser->rune) || parser->rune == '_' || parser->rune == '.')
			{
				if (parser->rune == '.')
				{
					switch (parser->token.tag)
					{
					case token_tag_binary:
					case token_tag_hexadecimal:
					case token_tag_decimal:
					case token_tag_scientific:
						parser_report_failure(parser, "Weird ass number.");
						while (!is_whitespace(parser->rune)) parser_advance(parser);
						goto failed;
					default:
						break;
					}
					parser->token.tag = token_tag_decimal;
				}
				parser_advance(parser);
			}
		}
		else
		{
			parser->token.tag = token_tag_unknown;
			parser_advance(parser);
			parser_report_failure(parser, "Unknown token.");
		}
	}
	
	parser->token.ending = parser->position;
	return parser->token.tag;

failed:
	parser->token.ending = parser->position;
	jump(*parser->failure_landing, 1);
}

static void parser_ensure_token(token_tag tag, parser *parser)
{
	if (parser->token.tag != tag)
	{
		parser_report_failure(parser, "Expected token: %s.", token_tags_representations[tag]);
		jump(*parser->failure_landing, 1);
	}
}

static void parser_expect_token(token_tag tag, parser *parser)
{
	parser_get_token(parser);
	parser_ensure_token(tag, parser);
}

typedef uint precedence;

constexpr precedence precedences[] =
{
	[node_tag_subexpression] = 0,
	[node_tag_indexation]    = 0,
	[node_tag_scope]         = 0,
	[node_tag_identifier]    = 0,
	[node_tag_text]          = 0,
	[node_tag_digital]       = 0,
	[node_tag_decimal]       = 0,

	[node_tag_procedure]     = 150,

	[node_tag_logical_negation] = 140,
	[node_tag_negation]         = 140,
	[node_tag_bitwise_negation] = 140,
	[node_tag_reference]        = 140,

	[node_tag_multiplication] = 130,
	[node_tag_division]       = 130,
	[node_tag_modulus]        = 130,

	[node_tag_addition]    = 120,
	[node_tag_subtraction] = 120,
	
	[node_tag_bitwise_left_shift]  = 110,
	[node_tag_bitwise_right_shift] = 110,

	[node_tag_logical_minority]           = 100,
	[node_tag_logical_majority]           = 100,
	[node_tag_logical_inclusive_minority] = 100,
	[node_tag_logical_inclusive_majority] = 100,

	[node_tag_logical_equality]   = 90,
	[node_tag_logical_inequality] = 90,
	
	[node_tag_bitwise_conjunction] = 80,
	
	[node_tag_bitwise_exclusive_disjunction] = 70,
	
	[node_tag_bitwise_disjunction] = 60,

	[node_tag_logical_conjunction] = 50,
	
	[node_tag_logical_disjunction] = 40,

	[node_tag_invocation] = 30,
	
	[node_tag_declaration] = 20,
	
	[node_tag_assignment]                               = 10,
	[node_tag_addition_assignment]                      = 10,
	[node_tag_subtraction_assignment]                   = 10,
	[node_tag_multiplication_assignment]                = 10,
	[node_tag_division_assignment]                      = 10,
	[node_tag_modulus_assignment]                       = 10,
	[node_tag_bitwise_conjunction_assignment]           = 10,
	[node_tag_bitwise_disjunction_assignment]           = 10,
	[node_tag_bitwise_exclusive_disjunction_assignment] = 10,
	[node_tag_bitwise_left_shift_assignment]            = 10,
	[node_tag_bitwise_right_shift_assignment]           = 10,
	[node_tag_condition]                                = 10,
	
	[node_tag_list] = 1,
};

static node *parser_parse_node(precedence precedence, parser *parser);

static void parser_parse_scope     (scope_node      *result, parser *parser);
static void parser_parse_identifier(identifier_node *result, parser *parser);
static void parser_parse_text      (text_node       *result, parser *parser);
static void parser_parse_digital   (digital_node    *result, parser *parser);
static void parser_parse_decimal   (decimal_node    *result, parser *parser);

node *parser_parse_node(precedence left_precedence, parser *parser)
{
	/* parse the _possibly left_ node */
	node *left = 0;
	{
		node_tag left_tag;
		switch (parser->token.tag)
		{
			/* pragma */
		case token_tag_octothorpe:
			parser_expect_token(token_tag_identifier, parser);
			{
				uint token_size = parser->token.ending - parser->token.beginning;
				utf8 pragma_identifier[token_size + 1];
				copy(pragma_identifier, parser->source + parser->token.beginning, token_size);
				pragma_identifier[token_size] = 0;
				pragma pragma = pragma_undefined;
				if (!compare_text(pragma_identifier, "fp64")) pragma = pragma_fp64;
			}
			break;
			
			/* scoped */
		case token_tag_left_parenthesis:    left_tag = node_tag_subexpression; goto scoped;
		case token_tag_left_square_bracket: left_tag = node_tag_indexation;    goto scoped;
		scoped:
			left = PUSH_TRAIN(node, unary_node, &parser->allocator);
			left->tag = left_tag;
			parser_get_token(parser); /* skip the onset */
			left->data->unary.node = parser_parse_node(0, parser);
			parser_ensure_token(left_tag == node_tag_subexpression ? token_tag_right_parenthesis : token_tag_right_square_bracket, parser);
			parser_get_token(parser);
			break;
			
			/* terminators */
		case token_tag_etx:
		case token_tag_semicolon:
		case token_tag_right_parenthesis:
		case token_tag_right_curly_bracket:
		case token_tag_right_square_bracket:
			goto finished;

			/* unary */
		case token_tag_exclamation_mark: left_tag = node_tag_logical_negation; goto unary;
		case token_tag_minus_sign:       left_tag = node_tag_negation;         goto unary;
		case token_tag_tilde:            left_tag = node_tag_bitwise_negation; goto unary;
		case token_tag_at_sign:          left_tag = node_tag_reference;        goto unary;
		unary:
			left = PUSH_TRAIN(node, unary_node, &parser->allocator);
			left->tag = left_tag;
			parser_get_token(parser); /* skip the operator */
			left->data->unary.node = parser_parse_node(precedences[left_tag], parser);
			break;

			/* scope */
		case token_tag_left_curly_bracket:
			if (left_precedence != precedences[node_tag_procedure])
			{
				left = PUSH_TRAIN(node, scope_node, &parser->allocator);
				left->tag = node_tag_scope;
				parser_parse_scope(&left->data->scope, parser);
			}
			break;

			/* identifier */
		case token_tag_identifier:
			left = PUSH_TRAIN(node, identifier_node, &parser->allocator);
			left->tag = node_tag_identifier;
			parser_parse_identifier(&left->data->identifier, parser);
			break;

			/* text */
		case token_tag_text:
			left = PUSH_TRAIN(node, text_node, &parser->allocator);
			left->tag = node_tag_text;
			parser_parse_text(&left->data->text, parser);
			break;

			/* digital */
		case token_tag_binary:
		case token_tag_digital:
		case token_tag_hexadecimal:
			left = PUSH_TRAIN(node, digital_node, &parser->allocator);
			left->tag = node_tag_digital;
			parser_parse_digital(&left->data->digital, parser);
			break;
			
			/* decimal */
		case token_tag_decimal:
		case token_tag_scientific:
			left = PUSH_TRAIN(node, decimal_node, &parser->allocator);
			left->tag = node_tag_decimal;
			parser_parse_decimal(&left->data->decimal, parser);
			break;

		case token_tag_equal_sign:
			/* a declaration's cast is omittable, and the only possible subsequent token
			   that may be a binary operator is an equal sign, so handle that case. */
			break;

		default:
			parser_report_failure(parser, "Unexpected token.");
			jump(*parser->failure_landing, 1);
		}
	}
	if (left)
	{
		for (;;)
		{
			bit is_ternary = 0;
			node_tag right_tag;
			switch (parser->token.tag)
			{
				/* terminators */
			case token_tag_etx:
			case token_tag_semicolon:
			case token_tag_right_parenthesis:
			case token_tag_right_curly_bracket:
			case token_tag_right_square_bracket:
				goto finished;

				/* logical */
			case token_tag_ampersand_2:                  right_tag = node_tag_logical_conjunction;        break;
			case token_tag_vertical_bar_2:               right_tag = node_tag_logical_disjunction;        break;
			case token_tag_equal_sign_2:                 right_tag = node_tag_logical_equality;           break;
			case token_tag_exclamation_mark_equal_sign:  right_tag = node_tag_logical_inequality;         break;
			case token_tag_greater_than_sign:            right_tag = node_tag_logical_majority;           break;
			case token_tag_less_than_sign:               right_tag = node_tag_logical_minority;           break;
			case token_tag_greater_than_sign_equal_sign: right_tag = node_tag_logical_inclusive_majority; break;
			case token_tag_less_than_sign_equal_sign:    right_tag = node_tag_logical_inclusive_minority; break;

				/* arithmetic */
			case token_tag_plus_sign:    right_tag = node_tag_addition;       break;
			case token_tag_minus_sign:   right_tag = node_tag_subtraction;    break;
			case token_tag_asterisk:     right_tag = node_tag_multiplication; break;
			case token_tag_slash:        right_tag = node_tag_division;       break;
			case token_tag_percent_sign: right_tag = node_tag_modulus;        break;

				/* bitwise */
			case token_tag_ampersand:           right_tag = node_tag_bitwise_conjunction;           break;
			case token_tag_vertical_bar:        right_tag = node_tag_bitwise_disjunction;           break;
			case token_tag_circumflex_accent:   right_tag = node_tag_bitwise_exclusive_disjunction; break;
			case token_tag_less_than_sign_2:    right_tag = node_tag_bitwise_left_shift;            break;
			case token_tag_greater_than_sign_2: right_tag = node_tag_bitwise_right_shift;           break;

				/* assignment */
			case token_tag_equal_sign:                     right_tag = node_tag_assignment;                               break;
			case token_tag_plus_sign_equal_sign:           right_tag = node_tag_addition_assignment;                      break;
			case token_tag_minus_sign_equal_sign:          right_tag = node_tag_subtraction_assignment;                   break;
			case token_tag_asterisk_equal_sign:            right_tag = node_tag_multiplication_assignment;                break;
			case token_tag_slash_equal_sign:               right_tag = node_tag_division_assignment;                      break;
			case token_tag_percent_sign_equal_sign:        right_tag = node_tag_modulus_assignment;                       break;
			case token_tag_ampersand_equal_sign:           right_tag = node_tag_bitwise_conjunction_assignment;           break;
			case token_tag_vertical_bar_equal_sign:        right_tag = node_tag_bitwise_disjunction_assignment;           break;
			case token_tag_circumflex_accent_equal_sign:   right_tag = node_tag_bitwise_exclusive_disjunction_assignment; break;
			case token_tag_less_than_sign_2_equal_sign:    right_tag = node_tag_bitwise_left_shift_assignment;            break;
			case token_tag_greater_than_sign_2_equal_sign: right_tag = node_tag_bitwise_right_shift_assignment;           break;

				/* other */
			case token_tag_full_stop:                    right_tag = node_tag_resolution;  break;
			case token_tag_comma:                        right_tag = node_tag_list;        break;
			case token_tag_colon:                        right_tag = node_tag_declaration; break;
			default:                                     right_tag = node_tag_invocation;  break;
			case token_tag_minus_sign_greater_than_sign: right_tag = node_tag_procedure;   goto ternary;
			case token_tag_question_mark:                right_tag = node_tag_condition;   goto ternary;
			ternary:
				is_ternary = 1;
				break;
			}

			precedence right_precedence = precedences[right_tag];
			if (right_precedence <= left_precedence) goto finished;

			/* skip the operator [if the syntax has one] */
			if (right_tag != node_tag_invocation) parser_get_token(parser);

			node *right = is_ternary ? PUSH_TRAIN(node, ternary_node, &parser->allocator) : PUSH_TRAIN(node, binary_node, &parser->allocator);
			right->tag = right_tag;
			right->data->binary.left = left;
			right->data->binary.right = parser_parse_node(right_precedence, parser);

			if (is_ternary)
			{
				switch (parser->token.tag)
				{
					/* procedure */
				case token_tag_left_curly_bracket:
					right->data->ternary.node = PUSH_TRAIN(node, scope_node, &parser->allocator);
					right->data->ternary.node->tag = node_tag_scope;
					parser_parse_scope(&right->data->ternary.node->data->scope, parser);
					break;

					/* condition */
				case token_tag_colon:
					parser_get_token(parser); /* skip the `:` */
					right->data->ternary.node = parser_parse_node(right_precedence, parser);
					break;

				default:
					/* the third node of a ternary is omittable, so just continue */
					break;
				}
			}

			left = right;
		}
	}

finished:
	return left;
}

static void display_node(const node *node, uint depth)
{
	uintb types[] =
	{
#define X(type, identifier, body, syntax) [node_tag_##identifier] = type,
	#include "code_nodes.inc"
#undef X
	};

	for (uint i = 0; i < depth; ++i) printf("  ");

	if (node)
	{
		printf("%s: ", node_tags_representations[node->tag]);

		depth += 1;
		switch (types[node->tag])
		{
		case 0:
			switch (node->tag)
			{
			case node_tag_scope:
				printf("\n");
				for (uint i = 0; i < node->data->scope.nodes_count; ++i)
				{
					display_node(node->data->scope.nodes[i], depth);
				}
				break;
			case node_tag_identifier:
				printf("%.*s", node->data->identifier.runes_count, node->data->identifier.runes);
				break;
			case node_tag_text:
				printf("%.*s", node->data->identifier.runes_count, node->data->text.runes);
				break;
			case node_tag_digital:
				printf("%llu", node->data->digital.value);
				break;
			case node_tag_decimal:
				printf("%lf", node->data->decimal.value);
				break;
			default:
				break;
			}
			break;
		case 1:
			printf("\n");
			display_node(node->data->unary.node, depth);
			break;
		case 2:
			printf("\n");
			display_node(node->data->binary.left, depth);
			display_node(node->data->binary.right, depth);
			break;
		case 3:
			printf("\n");
			display_node(node->data->ternary.left, depth);
			display_node(node->data->ternary.right, depth);
			display_node(node->data->ternary.node, depth);
			break;
		}
	}
	else
	{
		printf("undefined");
	}
	printf("\n");
}

void parser_parse_scope(scope_node *result, parser *parser)
{
	bit is_global = result == &parser->program->globe;
	if (!is_global) ASSERT(parser->token.tag == token_tag_left_curly_bracket);

	uint nodes_capacity = 8;
	result->nodes = ALLOCATE(node *, nodes_capacity); /* TODO: stop `VirtualAlloc`ing */
	result->nodes_count = 0;

	parser_get_token(parser); /* get the first token if `is_global`, otherwise, skip the `{` */
	for (;;)
	{
		node *current_node = parser_parse_node(0, parser);
		if (current_node)
		{
			if (result->nodes_count >= nodes_capacity)
			{
				uint additional_capacity = nodes_capacity / 2;
				node **new_memory = ALLOCATE(node *, nodes_capacity + additional_capacity);
				COPY(new_memory, result->nodes, result->nodes_count);
				DEALLOCATE(result->nodes, nodes_capacity);
				nodes_capacity += additional_capacity;
				result->nodes = new_memory;
			}
			result->nodes[result->nodes_count++] = current_node;
		}

		switch (parser->token.tag)
		{
		case token_tag_semicolon:
			parser_get_token(parser);
			break;
		case token_tag_etx:
			if (!is_global)
			{
				parser_report_failure(parser, "Unterminted %s.", token_tags_representations[token_tag_left_curly_bracket]);
				goto failed;
			}
			else goto finished;
		case token_tag_right_curly_bracket:
			if (is_global)
			{
				parser_report_failure(parser, "Extraneous %s.", token_tags_representations[token_tag_right_curly_bracket]);
				goto failed;
			}
			else goto finished;
		default:
			parser_report_caution(parser, "?????");
			UNIMPLEMENTED();
		}
	}

finished:
	parser_get_token(parser); /* skip `}`, or ignore ETX */

	return;

failed:
	jump(*parser->failure_landing, 1);
}

void parser_parse_identifier(identifier_node *result, parser *parser)
{
	result->runes_count = parser->token.ending - parser->token.beginning;
	const utf8 *source = parser->source + parser->token.beginning;
	result->runes = (utf8 *)push(result->runes_count, sizeof(void *), &parser->allocator);
	copy(result->runes, source, result->runes_count);
	parser_get_token(parser);
}

void parser_parse_text(text_node *result, parser *parser)
{
	ASSERT(parser->token.tag == token_tag_text);

	result->runes_count = parser->token.ending - parser->token.beginning;
	const utf8 *source = parser->source + parser->token.beginning;
	result->runes = (utf8 *)push(result->runes_count, sizeof(void *), &parser->allocator);
	copy(result->runes, source, result->runes_count);
	parser_get_token(parser);

	/* TODO: handle escape characters */
}

void parser_parse_digital(digital_node *result, parser *parser)
{
	ASSERT(parser->token.tag == token_tag_binary
	       || parser->token.tag == token_tag_digital
	       || parser->token.tag == token_tag_hexadecimal);

	uint base;
	switch (parser->token.tag)
	{
	case token_tag_binary:      base = 2;  break;
	case token_tag_digital:     base = 10; break;
	case token_tag_hexadecimal: base = 16; break;
	default: UNREACHABLE();
	}

	utf8 *ending;
	result->value = 0;
	uint token_size = parser->token.ending - parser->token.beginning;
	utf8 *source = parser->source + parser->token.beginning;
	for (uint i = 0; i < token_size; ++i)
	{
		utf8 character = source[i];
		if (character == '_') continue;
		result->value *= base;
		if (parser->token.tag == token_tag_hexadecimal) character -= character - '9' - 1;
		result->value += character - '0';
	}
	
	parser_get_token(parser);

	/* TODO: ditch the C standard library */
}

void parser_parse_decimal(decimal_node *result, parser *parser)
{
	ASSERT(parser->token.tag == token_tag_decimal
	       || parser->token.tag == token_tag_scientific);

	utf8 *ending;
	result->value = strtod(parser->source + parser->token.beginning, &ending);
	parser_get_token(parser);
	
	/* TODO: ditch the C standard library */
}

void parser_parse(const utf8 *source_path, program *program, parser *parser)
{
	ZERO(parser, 1);

	landing failure_landing;
	parser->failure_landing = &failure_landing;
	if (SET_LANDING(failure_landing))
	{
		REPORT_FAILURE("Failed to parse.");
		/* TODO: handle failure here */
		return;
	}

	parser->program = program;
	parser_load(source_path, parser);
	parser_parse_scope(&parser->program->globe, parser);
	for (uint i = 0; i < parser->program->globe.nodes_count; ++i)
	{
		display_node(parser->program->globe.nodes[i], 0);
	}

	

	REPORT_VERBOSE("Finished parsing.\n");
}
