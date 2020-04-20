#ifndef I4004_LEXER_H
#define I4004_LEXER_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "string.h"
#include "symtbl.h"
#include "error.h"

typedef struct {
	FILE *input;

	char *buffer;
	size_t cur_size;
	size_t idx;

	bool iseof;
	bool iserr;

	position pos;

	symtbl *symtbl;
} lexer_state;

enum token_type {
	TOK_EOF = 256,

	TOK_IDENT,
	TOK_NUMBER,
	TOK_DIRECTIVE,
	TOK_STRING,

	TOK_NEWLINE
};

typedef struct {
	enum token_type type;
	union {
		size_t ident;
		uint64_t num;
	};
	position pos;
} token;

lexer_state *lexer_start(FILE *input, const char *filename, symtbl *tbl);
token lexer_lex(lexer_state *state);
void lexer_end(lexer_state *state);

#endif //I4004_LEXER_H