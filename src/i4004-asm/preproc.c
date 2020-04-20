#include <malloc.h>
#include "preproc.h"

static void preproc_push(preproc_state *state, const char *filename, FILE *in_file) {
	input_file *file = malloc(sizeof(input_file));
	file->list.next = (list_head *) state->in_stack;
	file->lexer = lexer_start(in_file, filename, state->tbl);
	state->in_stack = file;
}

static void preproc_pop(preproc_state *state) {
	input_file *removed = state->in_stack;
	state->in_stack = (input_file *) state->in_stack->list.next;
	if(removed->lexer->iserr) state->iserr = true;
	lexer_end(removed->lexer);
	free(removed);
	state->is_newline = true;
}

preproc_state *preproc_create(const char *filename, FILE *in_file, symtbl *tbl) {
	preproc_state *state = malloc(sizeof(preproc_state));

	state->iserr = false;
	state->tbl = tbl;
	state->in_stack = 0;
	state->tok_list = state->tok_list_tail = malloc(sizeof(token_list));
	state->tok_list->list.next = 0;
	state->is_newline = true;

	preproc_push(state, filename, in_file);

	return state;
}

static token preproc_token_take(preproc_state *state) {
	token_list *st = (token_list*) state->tok_list->list.next;
	state->tok_list->list.next = st->list.next;
	if(!st->list.next) state->tok_list_tail = state->tok_list;
	token tok = st->tok;
	free(st);
	return tok;
}

static void preproc_token_add(preproc_state *state, token tok) {
	token_list *st = malloc(sizeof(token_list));
	state->tok_list_tail->list.next = (list_head *) st;
	st->list.next = 0;
	st->tok = tok;
	state->tok_list_tail = st;
}

static void preproc_directive(preproc_state *state) {
	token tok = lexer_lex(state->in_stack->lexer);
	if(tok.type != TOK_IDENT) {
		position_error(tok.pos, "expected preprocessor directive\n");
		state->iserr = true;
		return;
	}
	position_error(tok.pos, "unrecognized preprocessor directive: `%s`\n", symtbl_get(state->tbl, tok.ident));
	state->iserr = true;
}

static token preproc_handle(preproc_state *state) {
	token tok;
again:
	tok = lexer_lex(state->in_stack->lexer);

	switch(tok.type) {
		case TOK_EOF:
			if(state->in_stack->list.next) {
				preproc_pop(state);
				goto again;
			} else {
				return tok;
			}
		case TOK_NEWLINE:
			state->is_newline = true;
			return tok;
		default: break;
	}

	if(state->is_newline && tok.type == '%') {
		state->is_newline = false;
		preproc_directive(state);
		if(state->tok_list != state->tok_list_tail) return preproc_token_take(state);
		else goto again;
	}

	state->is_newline = false;
	return tok;
}

token preproc_token(preproc_state *state) {
	if(state->tok_list->list.next) return preproc_token_take(state);
	if(state->in_stack) return preproc_handle(state);

	fprintf(stderr, "error: no input files\n");
	state->iserr = true;

	return (token){.type = TOK_EOF};
}

void preproc_destroy(preproc_state *state) {
	list_head_destroy(state->tok_list_tail);
	while(state->in_stack) preproc_pop(state);
	free(state);
}
