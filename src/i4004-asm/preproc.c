#include <malloc.h>
#include <memory.h>
#include <libgen.h>
#include "preproc.h"

static void preproc_push(preproc_state *state, const char *filename, FILE *in_file) {
	input_file *file = malloc(sizeof(input_file));
	file->list.next = (list_head *) state->in_stack;
	file->lexer = lexer_start(in_file, filename, state->tbl);
	state->in_stack = file;
	state->is_newline = true;
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
	state->is_macro_recording = false;

	state->id_dir_macro = symtbl_ident(tbl, string_from("macro", 5));
	state->id_dir_endmacro = symtbl_ident(tbl, string_from("endmacro", 8));
	state->id_dir_include = symtbl_ident(tbl, string_from("include", 7));

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
	if(tok.ident == state->id_dir_macro) {
		position pos = tok.pos;
		tok = lexer_lex(state->in_stack->lexer);
		if(tok.type != TOK_IDENT) {
			position_error(tok.pos, "expected macro name after %%macro\n");
			state->iserr = true;
			return;
		}
		size_t name = tok.ident;
		tok = lexer_lex(state->in_stack->lexer);
		if(tok.type != TOK_NEWLINE) {
			position_error(tok.pos, "expected new line after macro declaration\n");
			state->iserr = true;
			return;
		}
		state->is_macro_recording = true;
		token_list *macro_record = malloc(sizeof(token_list));
		token_list *macro_record_tail = macro_record;
		macro_record->list.next = 0;
		macro_record->tok = tok;
		while(state->is_macro_recording) {
			tok = preproc_token(state);
			if(tok.type == TOK_EOF) {
				position_error(pos, "unterminated %%macro directive");
				state->iserr = true;
				exit(1);
			}
			token_list *next = malloc(sizeof(token_list));
			next->list.next = 0;
			next->tok = tok;
			macro_record_tail->list.next = (list_head *) next;
			macro_record_tail = next;
		}
		macro *m = malloc(sizeof(macro));
		m->name = name;
		m->list.next = (list_head *) state->macros;
		m->tokens = macro_record;
		state->macros = m;
		return;
	}
	if(tok.ident == state->id_dir_endmacro) {
		if(state->is_macro_recording) state->is_macro_recording = false;
		else {
			position_error(tok.pos, "%%endmacro without %%macro directive\n");
			state->iserr = true;
		}
		return;
	}
	if(tok.ident == state->id_dir_include) {
		position pos = tok.pos;
		tok = lexer_lex(state->in_stack->lexer);
		if(tok.type != TOK_STRING) {
			position_error(tok.pos, "expected \"FILENAME\"\n");
			state->iserr = true;
			return;
		}
		const char *filename_const = symtbl_get(state->tbl, tok.ident);
		char *cur_filename = strdup(state->in_stack->lexer->pos.filename);
		char *dir = dirname(cur_filename);
		string *include_file = string_from(dir, strlen(dir));
		include_file = string_append(include_file, '/');
		include_file = string_append_str(include_file, filename_const, strlen(filename_const));
		symtbl_ident(state->tbl, include_file);

		FILE *file = fopen(include_file->data, "r");
		if(!file) {
			position_error(pos, "could not open file `%s`\n", include_file->data);
			state->iserr = true;
			return;
		}
		preproc_push(state, include_file->data, file);
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
		case TOK_IDENT: {
				macro *m = state->macros;
				bool found = false;
				while(m) {
					if(m->name == tok.ident) {
						found = true;
						token_list *t = m->tokens;
						while(t) {
							preproc_token_add(state, t->tok);
							t = (token_list *) t->list.next;
						}
						break;
					}

					m = (macro *) m->list.next;
				}
				if(!found) return tok;
				else return preproc_token_take(state);
			}
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
	if(state->tok_list != state->tok_list_tail) return preproc_token_take(state);
	if(state->in_stack) return preproc_handle(state);

	fprintf(stderr, "error: no input files\n");
	state->iserr = true;

	return (token){.type = TOK_EOF};
}

void preproc_destroy(preproc_state *state) {
	list_head_destroy(state->macros);
	list_head_destroy(state->tok_list_tail);
	while(state->in_stack) preproc_pop(state);
	free(state);
}
