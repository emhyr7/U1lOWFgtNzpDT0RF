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

static inline void report_v(severity severity, parser *parser, const utf8 *message, vargs vargs) { report_source_v(severity, parser->source_path, parser->source, parser->token.beginning, parser->token.ending, parser->token.row, parser->token.column, message, vargs); }
static inline void report(severity severity, parser *parser, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); report_v(severity, parser, message, vargs); END_VARGS(vargs); }

static inline void report_verbose(parser *parser, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); report_v(severity_verbose, parser, message, vargs); END_VARGS(vargs); }
static inline void report_comment(parser *parser, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); report_v(severity_comment, parser, message, vargs); END_VARGS(vargs); }
static inline void report_caution(parser *parser, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); report_v(severity_caution, parser, message, vargs); END_VARGS(vargs); }
static inline void report_failure(parser *parser, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); report_v(severity_failure, parser, message, vargs); END_VARGS(vargs); }

static uintb peek(utf32 *rune, parser *parser)
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

static utf32 advance(parser *parser)
{
	utf32 rune;
	uintb increment = peek(&rune, parser);

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

static void load(const utf8 *source_path, parser *parser)
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
	advance(parser);
}

static token_tag get_token(parser *parser)
{
repeat:
	while (is_whitespace(parser->rune)) advance(parser);

	parser->token.beginning = parser->position;
	parser->token.row       = parser->row;
	parser->token.column    = parser->column;

	utf32 peeked_rune;
	uintb peeked_increment = peek(&peeked_rune, parser);

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
			do advance(parser);
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
			advance(parser);
			peeked_increment = peek(&peeked_rune, parser);
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
			advance(parser);
			peeked_increment = peek(&peeked_rune, parser);
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
			advance(parser);
			if (parser->rune == '"') break;
			if (parser->rune == '\\')
			{
				switch (advance(parser))
				{
				default:
					advance(parser);
					break;
				}
			}
		}
		advance(parser); /* skip the terminating `"` */
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
		advance(parser);
	advance_once:
		advance(parser);
		break;

	default:
		if (is_letter(parser->rune) || parser->rune == '_')
		{
			do advance(parser);
			while (is_letter(parser->rune) || parser->rune == '_' || parser->rune == '-' || is_digit(parser->rune));
			parser->token.tag = token_tag_identifier;
		}
		else if (is_digit(parser->rune))
		{
			parser->token.tag = token_tag_digital;
#if 0
			if (parser->rune == '0')
			{
				advance(parser);
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
						report_failure(parser, "Weird ass number.");
						while (!is_whitespace(parser->rune)) advance(parser);
						goto failed;
					default:
						break;
					}
					parser->token.tag = token_tag_decimal;
				}
				advance(parser);
			}
		}
		else
		{
			parser->token.tag = token_tag_unknown;
			advance(parser);
			report_failure(parser, "Unknown token.");
		}
	}
	
	parser->token.ending = parser->position;
	return parser->token.tag;

failed:
	parser->token.ending = parser->position;
	jump(*parser->failure_landing, 1);
}

static void ensure_token(token_tag tag, parser *parser)
{
	if (parser->token.tag != tag)
	{
		report_failure(parser, "Expected token: %s.", token_tags_representations[tag]);
		jump(*parser->failure_landing, 1);
	}
}

static void expect_token(token_tag tag, parser *parser)
{
	get_token(parser);
	ensure_token(tag, parser);
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
	[node_tag_address]          = 140,

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

static node *parse_node(precedence precedence, parser *parser);

static void parse_scope     (scope_node      *result, parser *parser);
static void parse_identifier(identifier_node *result, parser *parser);
static void parse_text      (text_node       *result, parser *parser);
static void parse_digital   (digital_node    *result, parser *parser);
static void parse_decimal   (decimal_node    *result, parser *parser);

node *parse_node(precedence left_precedence, parser *parser)
{
	uint beginning = parser->token.beginning;
	
	/* parse the _possibly left_ node */
	node *left = 0;
	{
		node_tag left_tag;
		switch (parser->token.tag)
		{
			/* pragma */
		case token_tag_octothorpe:
			left = PUSH_TRAIN(node, pragma_node, &parser->allocator);
			left->tag = node_tag_pragma;
			expect_token(token_tag_identifier, parser);
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
			get_token(parser); /* skip the onset */
			left->data->unary.node = parse_node(0, parser);
			ensure_token(left_tag == node_tag_subexpression ? token_tag_right_parenthesis : token_tag_right_square_bracket, parser);
			get_token(parser);
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
		case token_tag_at_sign:          left_tag = node_tag_address;          goto unary;
		unary:
			left = PUSH_TRAIN(node, unary_node, &parser->allocator);
			left->tag = left_tag;
			get_token(parser); /* skip the operator */
			left->data->unary.node = parse_node(precedences[left_tag], parser);
			break;

			/* scope */
		case token_tag_left_curly_bracket:
			if (left_precedence != precedences[node_tag_procedure])
			{
				left = PUSH_TRAIN(node, scope_node, &parser->allocator);
				left->tag = node_tag_scope;
				parse_scope(&left->data->scope, parser);
			}
			break;

			/* identifier */
		case token_tag_identifier:
			left = PUSH_TRAIN(node, identifier_node, &parser->allocator);
			left->tag = node_tag_identifier;
			parse_identifier(&left->data->identifier, parser);
			break;

			/* text */
		case token_tag_text:
			left = PUSH_TRAIN(node, text_node, &parser->allocator);
			left->tag = node_tag_text;
			parse_text(&left->data->text, parser);
			break;

			/* digital */
		case token_tag_binary:
		case token_tag_digital:
		case token_tag_hexadecimal:
			left = PUSH_TRAIN(node, digital_node, &parser->allocator);
			left->tag = node_tag_digital;
			parse_digital(&left->data->digital, parser);
			break;
			
			/* decimal */
		case token_tag_decimal:
		case token_tag_scientific:
			left = PUSH_TRAIN(node, decimal_node, &parser->allocator);
			left->tag = node_tag_decimal;
			parse_decimal(&left->data->decimal, parser);
			break;

		case token_tag_equal_sign:
			/* a declaration's cast is omittable, and the only possible subsequent token
			   that may be a binary operator is an equal sign, so handle that case. */
			break;

		default:
			report_failure(parser, "Unexpected token.");
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
			if (right_tag != node_tag_invocation) get_token(parser);

			node *right = is_ternary ? PUSH_TRAIN(node, ternary_node, &parser->allocator) : PUSH_TRAIN(node, binary_node, &parser->allocator);
			right->tag = right_tag;
			right->data->binary.left = left;
			right->data->binary.right = parse_node(right_precedence, parser);

			if (is_ternary)
			{
				switch (parser->token.tag)
				{
					/* procedure */
				case token_tag_left_curly_bracket:
					right->data->ternary.node = PUSH_TRAIN(node, scope_node, &parser->allocator);
					right->data->ternary.node->tag = node_tag_scope;
					parse_scope(&right->data->ternary.node->data->scope, parser);
					break;

					/* condition */
				case token_tag_colon:
					get_token(parser); /* skip the `:` */
					right->data->ternary.node = parse_node(right_precedence, parser);
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
	if (left)
	{
		left->beginning = beginning;
		left->ending = parser->token.ending;
	}
	return left;
}
uintb node_types[] =
{
#define X(type, identifier, body, syntax) [node_tag_##identifier] = type,
	#include "code_nodes.inc"
#undef X
};

static void display_node(const node *node, uint depth)
{
	for (uint i = 0; i < depth; ++i) printf("  ");

	if (node)
	{
		printf("%s: ", node_tags_representations[node->tag]);

		depth += 1;
		switch (node_types[node->tag])
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
				printf("%.*s", node->data->identifier.size, node->data->identifier.data);
				break;
			case node_tag_text:
				printf("%.*s", node->data->identifier.size, node->data->text.data);
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

void parse_scope(scope_node *result, parser *parser)
{
	bit is_global = result == &parser->program->globe;
	if (!is_global) ASSERT(parser->token.tag == token_tag_left_curly_bracket);

	uint nodes_capacity = 8;
	result->nodes = ALLOCATE(node *, nodes_capacity); /* TODO: stop `VirtualAlloc`ing */
	result->nodes_count = 0;

	get_token(parser); /* get the first token if `is_global`, otherwise, skip the `{` */
	for (;;)
	{
		node *current_node = parse_node(0, parser);
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
			get_token(parser);
			break;
		case token_tag_etx:
			if (!is_global)
			{
				report_failure(parser, "Unterminted %s.", token_tags_representations[token_tag_left_curly_bracket]);
				goto failed;
			}
			else goto finished;
		case token_tag_right_curly_bracket:
			if (is_global)
			{
				report_failure(parser, "Extraneous %s.", token_tags_representations[token_tag_right_curly_bracket]);
				goto failed;
			}
			else goto finished;
		default:
			report_caution(parser, "?????");
			UNIMPLEMENTED();
		}
	}

finished:
	get_token(parser); /* skip `}`, or ignore ETX */

	return;

failed:
	jump(*parser->failure_landing, 1);
}

void parse_identifier(identifier_node *result, parser *parser)
{
	result->size = parser->token.ending - parser->token.beginning;
	const utf8 *source = parser->source + parser->token.beginning;
	result->data = (utf8 *)push(result->size, sizeof(void *), &parser->allocator);
	COPY(result->data, source, result->size);
	get_token(parser);
}

void parse_text(text_node *result, parser *parser)
{
	ASSERT(parser->token.tag == token_tag_text);

	result->runes_count = parser->token.ending - parser->token.beginning;
	const utf8 *source = parser->source + parser->token.beginning;
	result->runes = (utf8 *)push(result->runes_count, sizeof(void *), &parser->allocator);
	copy(result->runes, source, result->runes_count);
	get_token(parser);

	/* TODO: handle escape characters */
}

void parse_digital(digital_node *result, parser *parser)
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
	
	get_token(parser);

	/* TODO: ditch the C standard library */
}

void parse_decimal(decimal_node *result, parser *parser)
{
	ASSERT(parser->token.tag == token_tag_decimal
	       || parser->token.tag == token_tag_scientific);

	utf8 *ending;
	result->value = strtod(parser->source + parser->token.beginning, &ending);
	get_token(parser);
	
	/* TODO: ditch the C standard library */
}

typedef uint type_handle;

typedef enum
{
	type_type_undefined,
	type_type_primitive,
	type_type_address,
	type_type_array,
	type_type_tuple,
	type_type_defined,
} type_type;

typedef enum : uintb
{
	type_type_uint8,
	type_type_uint16,
	type_type_uint32,
	type_type_uint64,
	type_type_sint8,
	type_type_sint16,
	type_type_sint32,
	type_type_sint64,
	type_type_real32,
	type_type_real64,
} primitive_type;

typedef struct
{
	type *subtype;
} address_type;

typedef struct
{
	type *subtype;
	uintl count;
} array_type;

typedef struct
{
	type **subtypes;
	uintl  subtypes_count;
} tuple_type;

typedef struct
{
	type *pointer;
} defined_type;

struct type
{
	type_type type;

	/* a type is addressable if and only if the type is of an indexation or leads
	   to a declaration */
	bit is_addressable : 1;

	union
	{
		primitive_type primitive;
		address_type   address;
		array_type     array;
		tuple_type     tuple;
		defined_type   defined;
	};
};

bit get_addressableness(const node *node)
{
	declaration_node *declaration = 0;

	bit is_addressable;
	switch (node->tag)
	{
		{
	case node_tag_resolution:
			declaration_node *declaration = get_declaration_from_resolution(node);
	case node_tag_identifier:
			if (!declaration ) declaration = get_declaration_from_identifier(node);
			is_addressable = declaration != 0;
		}
		break;
	case node_tag_declaration:
	case node_tag_indexation:
		is_addressable = 1;
		break;
	default:
		is_addressable = 0;
		break;
	}

	return is_addressable;
}

constexpr type primitive_type_sint8 =
{
	.type = type_type_literal,
	.type = type_type_
};

typedef uint type_identifier;

typedef struct
{
	void *key;
	uint  key_size;
} table_key;

typedef struct
{
	table_key *keys;
	byte      *values;
	uint       entries_count;
} table;

void *get_from_table(uint value_size, const void *key, uint key_size, table *table)
{
	void *value = 0;
	for (uint i = 0; i < table->entries_count; ++i)
	{
		table_key *table_key = &table->keys[i];
		if (!COMPARE(key, table_key->key, MINIMUM(table_key->key_size, key_size)))
		{
			value = &table->values[i * value_size];
			break;
		}
	}
	return value;
}

#define GET_FROM_TABLE(type, key, key_size, table) (type *)get_from_table(sizeof(type), key, key_size, table)

/* for uniformality */
bit type_is_addressable(const type *type)
{
	return type->is_addressable;
}

node *set_type_of_literal_node(node *node, parser *parser)
{
	switch (node->tag)
	{
	case node_tag_scope:
		node->type_info.class = type_class_null;
		break;
	case node_tag_identifier:
		/* NOTE: the identifier might be the left of a declaration, instead of a path
		   to a declaration... */
		break;
	case node_tag_text:
		node->type_info.class = type_class_array;
		node->type_info.pointer = push_type(node->type.class);
		node->type_info.pointer->array.subtype = &uint8_type;
		node->type_info.pointer->array.size = node->data->text.size;
		break;
	case node_tag_digital:
		node->type_info.class = type_class_primitive;
		switch (get_minimum_bits_count_of(&node->data->digital.value))
		{
		case 8:  node->type_info.pointer = &primitive_types.uint8;  break;
		case 16: node->type_info.pointer = &primitive_types.uint16; break;
		case 32: node->type_info.pointer = &primitive_types.uint32; break;
		case 64: node->type_info.pointer = &primitive_types.uint64; break;
		}
		break;
	case node_tag_decimal:
		node->type_info.class = type_class_primitive;
		node->type_info.pointer = &primitive_types.real64;
		break;
	default:
		UNREACHABLE();
	}
}

node *set_type_of_unary_node(node *node, parser *parser)
{
	ZERO(&node->type, 1);

	node *subnode = set_type_of_node(node->data->unary.node, parser);

	switch (node->tag)
	{
	case node_tag_logical_negation:
		if (type_is_integer(&subnode->type))
		{
			result->type = subnode->type;
		}
		else
		{
			report_failure(parser, "A logical negation only applies to integers.");
			goto failed;
		}
		break;
	case node_tag_negation:
		if (type_is_integer(&subnode->type))
		{
			if (type_is_unsigned(&subnode->type))
			{
				uint bits_count = get_bits_count_of_type(&subnode->type);
				switch (bits_count)
				{
				case 8:  node->type = &sint8_type;  break;
				case 16: node->type = &sint16_type; break;
				case 32: node->type = &sint32_type; break;
				case 64: node->type = &sint64_type; break;
				}
			}
			else
			{
				result->type = subnode->type;
			}
		}
		else if (type_is_decimal(&subnode->type))
		{
			node->type = subnode->type;
		}
		else
		{
			report_failure(parser, "A negation only applies to numbers.");
			goto failed;
		}
		break;
	case node_tag_bitwise_negation:
		if (type_is_integer(&subnode->type))
		{
			node->type = subnode->type;
		}
		else
		{
			report_failure(parser, "A bitwise negation only applies to integers.");
			goto failed;
		}
		break;
	case node_tag_address:
		if (type_is_addressable(subnode->type))
		{
			node->type = push_type(type_type_address);
			node->type->address.subtype = subnode->type;
		}
		else
		{
			report_failure(parser, "A address only applies to addressables.");
			goto failed;
		}
		break;
	default:
		UNREACHABLE();
	}

	return node;
}

void parse(const utf8 *source_path, program *program, parser *parser)
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
	load(source_path, parser);
	parse_scope(&parser->program->globe, parser);

	REPORT_VERBOSE("Finished parsing.\n");
}
