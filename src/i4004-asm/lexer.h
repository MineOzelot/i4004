#ifndef I4004_LEXER_H
#define I4004_LEXER_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "string.h"
#include "symtbl.h"

typedef struct {
	FILE *input;

	char *buffer;
	size_t cur_size;
	size_t idx;

	bool iseof;
	bool iserr;

	size_t column, line;

	symtbl *symtbl;
} lexer_state;

enum token_type {
	tok_eof,

	tok_ident,
	tok_number,
	tok_directive,

	tok_semi,
	tok_comma,
	tok_newline
};

typedef struct {
	enum token_type type;
	union {
		size_t ident;
		uint64_t num;
	};
	size_t column, line;
} token;

lexer_state *lexer_start(FILE *input);
token lexer_lex(lexer_state *state);
void lexer_end(lexer_state *state);

#endif //I4004_LEXER_H