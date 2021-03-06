#include <malloc.h>
#include <ctype.h>

#include "lexer.h"
#include "error.h"

#define LEXER_BUFSIZ BUFSIZ

inline static char lexer_getch(lexer_state *state) {
	return state->buffer[state->idx];
}

static char lexer_nextch(lexer_state *state) {
	if(lexer_getch(state) == '\n') {
		state->pos.line++;
		state->pos.column = 0;
	}
	if(state->idx >= state->cur_size) {
		state->cur_size = fread(state->buffer, sizeof(char), LEXER_BUFSIZ, state->input);

		if(state->cur_size == 0) {
			state->iseof = true;
		}

		state->idx = 0;
	} else {
		state->idx++;
		state->pos.column++;
	}
	return lexer_getch(state);
}

lexer_state *lexer_start(FILE *input, const char *filename, symtbl *tbl) {
	lexer_state *state = malloc(sizeof(lexer_state));

	state->input = input;
	state->buffer = calloc(LEXER_BUFSIZ, sizeof(char));
	state->cur_size = 0;
	state->idx = LEXER_BUFSIZ;
	state->pos.column = 0;
	state->pos.line = 1;
	state->pos.filename = filename;

	state->iseof = false;
	state->iserr = false;

	state->symtbl = tbl;

	lexer_nextch(state);

	return state;
}

static bool is_ident(char ch) {
	if(isalnum(ch)) return true;
	return ch == '_' || ch == '.';
}

static token lexer_ident(lexer_state *state) {
	position pos = state->pos;
    string *str = string_empty();
	str = string_append(str, lexer_getch(state));

	while(is_ident(lexer_nextch(state))) {
		str = string_append(str, lexer_getch(state));
	}

	size_t ident = symtbl_ident(state->symtbl, str);

	return (token) {.type = TOK_IDENT, .ident = ident, .pos = pos};
}

static token lexer_directive(lexer_state *state) {
	if(!isalnum(lexer_nextch(state))) {
		position_error(state->pos, "empty directive\n");
		state->iserr = true;
		return (token) {.type = TOK_EOF, .pos = state->pos};
	}
	token ident_tok = lexer_ident(state);
	ident_tok.type = TOK_DIRECTIVE;
	return ident_tok;
}

static bool is_digit(int base, char ch) {
	if(base == 10) return isdigit(ch) != 0;
	if(base == 16) return isxdigit(ch) != 0;
	if(base == 8) return ch >= '0' && ch <= '7';
	if(base == 2) return ch == '0' || ch == '1';
	return false;
}

static token lexer_number(lexer_state *state) {
	int base = 10;
	position pos = state->pos;

	if(lexer_getch(state) == '0') {
		char after_zero = lexer_nextch(state);
		if(after_zero == 'x') base = 16;
		else if(after_zero == 'b') base = 2;
		else if(is_digit(8, after_zero)) base = 8;
		else return (token) {.type = TOK_NUMBER, .num = 0, .pos = pos};
		if(base != 8) lexer_nextch(state);
	}

	if(!is_digit(base, lexer_getch(state))) {
		const char *base_str = "?";
		if(base == 10) base_str = "decimal";
		else if(base == 16) base_str = "hexadecimal";
		else if(base == 8) base_str = "octal";
		else if(base == 2) base_str = "binary";
		position_error(state->pos, "expected %s number\n", base_str);
		state->iserr = true;
		return (token) {.type = TOK_EOF, .pos = state->pos};
	}

	string *str = string_empty();
	str = string_append(str, lexer_getch(state));

	while(is_digit(base, lexer_nextch(state))) {
		str = string_append(str, lexer_getch(state));
	}

	char *end = str->data + str->len;
	unsigned long long number = strtoull(str->data, &end, base);

	string_destroy(str);

 	return (token) {.type = TOK_NUMBER, .num = number, .pos = pos};
}

static bool is_punctuation(char ch) {
	return (ch == ',' || ch == ':' || ch == '%');
}

token lexer_lex(lexer_state *state) {
	if(state->iseof) return (token) {.type = TOK_EOF, .pos = state->pos};
	if(lexer_getch(state) == '\n') {
		position pos = state->pos;
		lexer_nextch(state);
		return (token) {.type = TOK_NEWLINE, .pos = pos};
	}
	while(isspace(lexer_getch(state))) lexer_nextch(state);

	if(lexer_getch(state) == ';') {
		while(lexer_nextch(state) != '\n' && !state->iseof);
		position pos = state->pos;
		lexer_nextch(state);
		return (token) {.type = TOK_NEWLINE, .pos = pos};
	}
	if(lexer_getch(state) == '"') {
		string *str = string_empty();
		position pos = state->pos;
		while(lexer_nextch(state) != '"') {
			if(state->iseof) {
				position_error(state->pos, "missing terminating `\"` character\n");
				state->iserr = true;
				string_destroy(str);
				return (token){.type = TOK_EOF, .pos = state->pos};
			}
			string_append(str, lexer_getch(state));
		}
		lexer_nextch(state);
		size_t sym = symtbl_ident(state->symtbl, str);
		return (token){.type = TOK_STRING, .pos = pos, .ident = sym};
	}

	if(isalpha(lexer_getch(state))) return lexer_ident(state);
	if(isdigit(lexer_getch(state))) return lexer_number(state);
	if(is_punctuation(lexer_getch(state))) {
		position pos = state->pos;
		char ch = lexer_getch(state);
		lexer_nextch(state);
		return (token) {.type = (enum token_type) ch, .pos = pos};
	}
	if(lexer_getch(state) == '.') return lexer_directive(state);

	if(lexer_getch(state) == '\0') return (token) {.type = TOK_EOF, .pos = state->pos};

	state->iserr = true;
	position_error(state->pos, "unrecognized character: `%c`\n", lexer_getch(state));
	return (token) {.type = TOK_EOF, .pos = state->pos};
}

void lexer_end(lexer_state *state) {
	fclose(state->input);
	free(state->buffer);
	free(state);
}