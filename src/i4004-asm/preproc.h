#ifndef I4004_PREPROC_H
#define I4004_PREPROC_H

#include <stdio.h>
#include <stdbool.h>

#include "list_head.h"
#include "error.h"
#include "string.h"
#include "lexer.h"

typedef struct {
	list_head list;
	lexer_state *lexer;
} input_file;

typedef struct token_list token_list;
struct token_list {
	list_head list;
	token tok;
};

typedef struct {
	list_head list;
	size_t name;
	token_list *tokens;
} macro;

typedef struct {
	symtbl *tbl;
	input_file *in_stack;

	token_list *tok_list;
	token_list *tok_list_tail;

	bool is_newline;
	bool iserr;

	bool is_macro_recording;

	macro *macros;

	size_t id_dir_macro, id_dir_endmacro;
	size_t id_dir_include;
} preproc_state;

preproc_state *preproc_create(const char *filename, FILE *in_file, symtbl *tbl);

token preproc_token(preproc_state *state);

void preproc_destroy(preproc_state *state);

#endif //I4004_PREPROC_H
