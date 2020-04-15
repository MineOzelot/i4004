#include <malloc.h>
#include <ctype.h>

#include "lexer.h"

#define LEXER_BUFSIZ BUFSIZ

inline static char lexer_getch(lexer_state *state) {
	return state->buffer[state->idx];
}

static char lexer_nextch(lexer_state *state) {
	if(lexer_getch(state) == '\n') {
		state->line++;
		state->column = 0;
	}
	if(state->idx >= state->cur_size) {
		state->cur_size = fread(state->buffer, sizeof(char), LEXER_BUFSIZ, state->input);

		if(state->cur_size == 0) {
			state->iseof = true;
		}

		state->idx = 0;
	} else {
		state->idx++;
		state->column++;
	}
	return lexer_getch(state);
}

lexer_state *lexer_start(FILE *input) {
	lexer_state *state = malloc(sizeof(lexer_state));

	state->input = input;
	state->buffer = calloc(LEXER_BUFSIZ, sizeof(char));
	state->cur_size = 0;
	state->idx = LEXER_BUFSIZ;
	state->column = 0;
	state->line = 1;

	state->iseof = false;
	state->iserr = false;

	state->symtbl = symtbl_create();

	lexer_nextch(state);

	return state;
}

static bool is_ident(char ch) {
	if(isalnum(ch)) return true;
	return ch == '_' || ch == '.';
}

static token lexer_ident(lexer_state *state) {
	size_t line = state->line, col = state->column;
    string *str = string_empty();
	str = string_append(str, lexer_getch(state));

	while(is_ident(lexer_nextch(state))) {
		str = string_append(str, lexer_getch(state));
	}

	size_t ident = symtbl_ident(state->symtbl, str);

	return (token) {.type = tok_ident, .ident = ident, .line = line, .column = col};
}

static token lexer_directive(lexer_state *state) {
	if(!isalnum(lexer_nextch(state))) {
		fprintf(stderr, "error: empty directive at line %zu, column %zu\n",
		        state->line, state->column - 1
		);
		state->iserr = true;
		return (token) {.type = tok_eof, .line = state->line, .column = state->column};
	}
	token ident_tok = lexer_ident(state);
	ident_tok.type = tok_directive;
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
	size_t line = state->line, column = state->column;

	if(lexer_getch(state) == '0') {
		char after_zero = lexer_nextch(state);
		if(after_zero == 'x') base = 16;
		else if(after_zero == 'b') base = 2;
		else if(is_digit(8, after_zero)) base = 8;
		else return (token) {.type = tok_number, .num = 0, .line = line, .column = column};
		if(base != 8) lexer_nextch(state);
	}

	if(!is_digit(base, lexer_getch(state))) {
		const char *base_str = "?";
		if(base == 10) base_str = "decimal";
		else if(base == 16) base_str = "hexadecimal";
		else if(base == 8) base_str = "octal";
		else if(base == 2) base_str = "binary";
		fprintf(stderr, "error: expected %s number at line %zu, column %zu\n",
		        base_str, state->line, state->column
		);
		state->iserr = true;
		return (token) {.type = tok_eof, .line = state->line, .column = state->column};
	}

	string *str = string_empty();
	str = string_append(str, lexer_getch(state));

	while(is_digit(base, lexer_nextch(state))) {
		str = string_append(str, lexer_getch(state));
	}

	char *end = str->data + str->len;
	unsigned long long number = strtoull(str->data, &end, base);

	string_destroy(str);

 	return (token) {.type = tok_number, .num = number, .line = line, .column = column};
}

token lexer_lex(lexer_state *state) {
	if(state->iseof) return (token) {.type = tok_eof, .line = state->line, .column = state->column};
	if(lexer_getch(state) == '\n') {
		size_t line = state->line, col = state->column;
		lexer_nextch(state);
		return (token) {.type = tok_newline, .line = line, .column = col};
	}
	while(isspace(lexer_getch(state))) lexer_nextch(state);

	if(lexer_getch(state) == ';') {
		while(lexer_nextch(state) != '\n' && !state->iseof);
		size_t line = state->line, col = state->column;
		lexer_nextch(state);
		return (token) {.type = tok_newline, .line = line, .column = col};
	}

	if(isalpha(lexer_getch(state))) return lexer_ident(state);
	if(isdigit(lexer_getch(state))) return lexer_number(state);
	if(lexer_getch(state) == ',') {
		lexer_nextch(state);
		return (token) {.type = tok_comma, .line = state->line, .column = state->column - 1};
	}
	if(lexer_getch(state) == ':') {
		lexer_nextch(state);
		return (token) {.type = tok_semi, .line = state->line, .column = state->column - 1};
	}
	if(lexer_getch(state) == '.') return lexer_directive(state);

	if(lexer_getch(state) == '\0') return (token) {.type = tok_eof, .line = state->line, .column = state->column};

	state->iserr = true;
	fprintf(stderr, "error: unrecognized character: `%c` (line %zu, column %zu)\n",
	        lexer_getch(state), state->line, state->column
	);
	return (token) {.type = tok_eof, .line = state->line, .column = state->column};
}

void lexer_end(lexer_state *state) {
	fclose(state->input);
	free(state->buffer);
	symtbl_destroy(state->symtbl);
	free(state);
}