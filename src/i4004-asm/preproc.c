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

static token preproc_take(preproc_state *state) ;
static enum token_type preproc_next(preproc_state *state) {
eof:
	state->tok = lexer_lex(state->in_stack->lexer);
	if(state->tok.type == TOK_EOF) {
		if(state->in_stack->list.next) {
			preproc_pop(state);
			goto eof;
		}
	}

	return state->tok.type;
}

static token preproc_take(preproc_state *state) {
	token tok = state->tok;
	preproc_next(state);
	return tok;
}

static void preproc_expect_newline(preproc_state *state) {
	if(state->tok.type != TOK_NEWLINE && state->tok.type != TOK_EOF) {
		position_error(state->tok.pos, "expected new line\n");
		state->iserr = true;
	} else {
		preproc_next(state);
	}
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

	preproc_next(state);

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

static ident_list *preproc_param(preproc_state *state) {
	if(state->tok.type == TOK_NEWLINE) return 0;
	if(state->tok.type != TOK_IDENT) {
		position_error(state->tok.pos, "expected parameter identifier\n");
		state->iserr = true;
		return 0;
	}
	ident_list *id = malloc(sizeof(ident_list));
	id->pos = state->tok.pos;
	id->list.next = 0;
	id->ident = state->tok.ident;
	preproc_next(state);
	return id;
}

static ident_list *preproc_paramlist(preproc_state *state) {
	ident_list *first = preproc_param(state);
	ident_list *cur = first;
	while(state->tok.type == ',') {
		preproc_next(state);
		ident_list *next = preproc_param(state);
		if(!next) {
			position_error(state->tok.pos, "expected parameter\n");
			list_head_destroy(first);
			state->iserr = true;
			return 0;
		}
		cur->list.next = (list_head *) next;
		cur = next;
	}
	return first;
}

static void preproc_directive(preproc_state *state) {
	if(state->tok.type != TOK_IDENT) {
		position_error(state->tok.pos, "expected preprocessor directive\n");
		state->iserr = true;
		return;
	}
	if(state->tok.ident == state->id_dir_macro) {
		position pos = state->tok.pos;
		if(state->is_macro_recording) {
			position_error(pos, "macro directive inside of another macro\n");
			state->iserr = true;
			return;
		}
		preproc_next(state);
		if(state->tok.type != TOK_IDENT) {
			position_error(state->tok.pos, "expected macro name after %%macro\n");
			state->iserr = true;
			return;
		}
		size_t name = state->tok.ident;
		preproc_next(state);
		ident_list *params = preproc_paramlist(state);
		if(state->tok.type != TOK_NEWLINE) {
			position_error(state->tok.pos, "expected new line after macro declaration\n");
			state->iserr = true;
			list_head_destroy(params);
			return;
		}
		state->is_macro_recording = true;
		token_list *macro_record = malloc(sizeof(token_list));
		token_list *macro_record_tail = macro_record;
		macro_record->list.next = 0;
		macro_record->tok = state->tok;
		while(true) {
			token tok = preproc_token(state);
			if(!state->is_macro_recording) break;
			if(tok.type == TOK_EOF && state->is_macro_recording) {
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
		m->params = params;
		state->macros = m;
		return;
	}
	if(state->tok.ident == state->id_dir_endmacro) {
		if(state->is_macro_recording) state->is_macro_recording = false;
		else {
			position_error(state->tok.pos, "%%endmacro without %%macro directive\n");
			state->iserr = true;
		}
		preproc_next(state);
		preproc_expect_newline(state);
		return;
	}
	if(state->tok.ident == state->id_dir_include) {
		position pos = state->tok.pos;
		preproc_next(state);
		if(state->tok.type != TOK_STRING) {
			position_error(state->tok.pos, "expected \"FILENAME\"\n");
			state->iserr = true;
			return;
		}
		const char *filename_const = symtbl_get(state->tbl, state->tok.ident);
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
		preproc_next(state);
		return;
	}
	position_error(state->tok.pos, "unrecognized preprocessor directive: `%s`\n", symtbl_get(state->tbl, state->tok.ident));
	state->iserr = true;
	preproc_next(state);
}

static replace_list *preproc_macro_arg(preproc_state *state) {
	replace_list *rep = malloc(sizeof(replace_list));
	rep->list.next = 0;
	rep->tokens = 0;
	if(state->tok.type == ',' || state->tok.type == TOK_NEWLINE || state->tok.type == TOK_EOF) {
		position_error(state->tok.pos, "empty macro parameter\n");
		state->iserr = true;
		return rep;
	}
	token_list *tokens = malloc(sizeof(token_list));
	tokens->list.next = 0;
	tokens->tok = state->tok;
	token_list *cur = tokens;
	while(preproc_next(state) != ',' && state->tok.type != TOK_NEWLINE && state->tok.type != TOK_EOF) {
		token_list *next = malloc(sizeof(token_list));
		next->list.next = 0;
		next->tok = state->tok;
		cur->list.next = (list_head *) next;
		cur = next;
	}
	rep->tokens = tokens;
	return rep;
}

static replace_list *preproc_macro_arglist(preproc_state *state) {
	if(state->tok.type == TOK_NEWLINE || state->tok.type == TOK_EOF) return 0;
	replace_list *reps = preproc_macro_arg(state);
	replace_list *cur = reps;
	while(state->tok.type == ',') {
		preproc_next(state);
		replace_list *next = preproc_macro_arg(state);
		cur->list.next = (list_head *) next;
		cur = next;
	}
	return reps;
}

static void preproc_macro(preproc_state *state, macro *m) {
	position pos = state->tok.pos;
	replace_list *reps = preproc_macro_arglist(state);
	preproc_expect_newline(state);
	size_t params_count = list_head_size(m->params);
	size_t args_count = list_head_size(reps);

	if(params_count != args_count) {
		position_error(pos, "expected %zu arguments, given %zu\n", params_count, args_count);
		state->iserr = true;
		return;
	}

	replace_list *it_rep = reps;
	ident_list *it_ident = m->params;
	while(it_rep) {
		it_rep->ident = it_ident->ident;

		it_rep = (replace_list *) it_rep->list.next;
		it_ident = (ident_list *) it_ident->list.next;
	}

	token_list *t = m->tokens;
	while(t) {
		if(t->tok.type == TOK_IDENT) {
			it_rep = reps;
			bool found = false;
			while(it_rep) {
				if(it_rep->ident == t->tok.ident) {
					found = true;
					token_list *it_tok = it_rep->tokens;
					while(it_tok) {
						preproc_token_add(state, it_tok->tok);
						it_tok = (token_list *) it_tok->list.next;
					}
					break;
				}
				it_rep = (replace_list *) it_rep->list.next;
			}
			if(!found) preproc_token_add(state, t->tok);
		} else {
			preproc_token_add(state, t->tok);
		}
		t = (token_list *) t->list.next;
	}
	it_rep = reps;
	while(it_rep) {
		list_head_destroy(it_rep->tokens);
		it_rep = (replace_list *) it_rep->list.next;
	}
	list_head_destroy(reps);
}

static token preproc_handle(preproc_state *state) {
again:
	switch(state->tok.type) {
		case TOK_NEWLINE:
			state->is_newline = true;
			return preproc_take(state);
		case TOK_IDENT: {
				macro *m = state->macros;
				bool found = false;
				while(m) {
					if(m->name == state->tok.ident) {
						found = true;
						preproc_next(state);
						preproc_macro(state, m);
						break;
					}

					m = (macro *) m->list.next;
				}
				if(!found) return preproc_take(state);
				else return preproc_token_take(state);
			}
		default: break;
	}

	if(state->is_newline && state->tok.type == '%') {
		state->is_newline = false;
		preproc_next(state);
		preproc_directive(state);
		if(state->tok_list != state->tok_list_tail) return preproc_token_take(state);
		else goto again;
	}

	state->is_newline = false;
	return preproc_take(state);
}

token preproc_token(preproc_state *state) {
	if(state->tok_list != state->tok_list_tail) return preproc_token_take(state);
	if(state->in_stack) return preproc_handle(state);

	fprintf(stderr, "error: no input files\n");
	state->iserr = true;

	return (token){.type = TOK_EOF};
}

void preproc_destroy(preproc_state *state) {
	while(state->macros) {
		list_head_destroy(state->macros->tokens);
		list_head_destroy(state->macros->params);
		state->macros = (macro *) state->macros->list.next;
	}
	list_head_destroy(state->macros);
	list_head_destroy(state->tok_list_tail);
	while(state->in_stack) preproc_pop(state);
	free(state);
}
