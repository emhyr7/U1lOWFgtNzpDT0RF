#include "code.h"

#include "base.c"

static void v_report_source(severity severity, const utf8 *source_path, const utf8 *source, uint beginning, uint ending, uint row, uint column, const utf8 *message, vargs vargs)
{
	fprintf(stderr, "[%s] %s(%u|%u:%u): ", severity_representations[severity], source_path, beginning, row, column);
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

static inline void report_source(severity severity, const utf8 *source_path, const utf8 *source, uint beginning, uint ending, uint row, uint column, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); v_report_source(severity, source_path, source, beginning, ending, row, column, message, vargs); END_VARGS(vargs); }

static inline void report_source_verbose(const utf8 *source_path, const utf8 *source, uint beginning, uint ending, uint row, uint column, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); v_report_source(severity_verbose, source_path, source, beginning, ending, row, column, message, vargs); END_VARGS(vargs); }
static inline void report_source_comment(const utf8 *source_path, const utf8 *source, uint beginning, uint ending, uint row, uint column, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); v_report_source(severity_comment, source_path, source, beginning, ending, row, column, message, vargs); END_VARGS(vargs); }
static inline void report_source_caution(const utf8 *source_path, const utf8 *source, uint beginning, uint ending, uint row, uint column, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); v_report_source(severity_caution, source_path, source, beginning, ending, row, column, message, vargs); END_VARGS(vargs); }
static inline void report_source_failure(const utf8 *source_path, const utf8 *source, uint beginning, uint ending, uint row, uint column, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); v_report_source(severity_failure, source_path, source, beginning, ending, row, column, message, vargs); END_VARGS(vargs); }

static inline void v_parser_report(severity severity, parser *parser, const utf8 *message, vargs vargs) { v_report_source(severity, parser->source_path, parser->source, parser->token.beginning, parser->token.ending, parser->token.row, parser->token.column, message, vargs); }
static inline void parser_report(severity severity, parser *parser, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); v_parser_report(severity, parser, message, vargs); END_VARGS(vargs); }

static inline void parser_report_verbose(parser *parser, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); v_parser_report(severity_verbose, parser, message, vargs); END_VARGS(vargs); }
static inline void parser_report_comment(parser *parser, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); v_parser_report(severity_comment, parser, message, vargs); END_VARGS(vargs); }
static inline void parser_report_caution(parser *parser, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); v_parser_report(severity_caution, parser, message, vargs); END_VARGS(vargs); }
static inline void parser_report_failure(parser *parser, const utf8 *message, ...) { vargs vargs; GET_VARGS(vargs, message); v_parser_report(severity_failure, parser, message, vargs); END_VARGS(vargs); }

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
	handle source_file = open_file(parser->source_path);
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
	while (is_whitespace(parser->rune)) parser_advance(parser);

repeat:
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
		parser->token.tag = token_tag_string;
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
		parser_report_failure(parser, "Expected token: %s.", token_tag_representations[tag]);
		jump(*parser->failure_landing, 1);
	}
}

static void parser_expect_token(token_tag tag, parser *parser)
{
	parser_get_token(parser);
	parser_ensure_token(tag, parser);
}

static node *parser_parse_node(parser *parser)
{
	node *result = 0;
	return result;
}

static void parser_parse_scope(scope_node *result, parser *parser)
{
}

static void parser_parse_identifier(identifier_node *result, parser *parser)
{
}

static void parser_parse_string(string_node *result, parser *parser)
{
}

static void parser_parse_digital(digital_node *result, parser *parser)
{
}

static void parser_parse_decimal(decimal_node *result, parser *parser)
{
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

	parser_load(source_path, parser);

	for (token_tag tag = parser_get_token(parser); tag != token_tag_etx; tag = parser_get_token(parser))
	{
		parser_report_verbose(parser, "%s", token_tag_representations[tag]);
	}
	
	//parser_parse_scope(&parser->program->globe, parser);

	REPORT_VERBOSE("Finished parsing.\n");
}
