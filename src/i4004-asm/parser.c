#include "parser.h"
#include "lexer.h"

parser_state *parser_start(lexer_state *lexer) {
	parser_state *state = malloc(sizeof(parser_state));

	state->lexer = lexer;
	state->iserr = false;
	state->isfailed = false;

	state->insns_head = state->insns_tail = malloc(sizeof(insn));

	state->dirs = 0;

	state->kw_dir_org = symtbl_ident(lexer->symtbl, string_from("org", 3));

	return state;
}

static enum token_type parser_next(parser_state *state) {
	state->tok = state->lookahead;
	state->lookahead = lexer_lex(state->lexer);
	if(state->lexer->iserr) state->isfailed = true;
	return state->tok.type;
}

static enum token_type parser_next_twice(parser_state *state) {
	state->tok = lexer_lex(state->lexer);
	state->lookahead = lexer_lex(state->lexer);
	if(state->lexer->iserr) state->isfailed = true;
	return state->tok.type;
}

static void parser_recover(parser_state *state) {
	state->iserr = true;
	while(parser_next(state) != tok_newline && state->tok.type != tok_eof);
}

/* [label:]* */
static label *parser_label_list(parser_state *state) {
	label *lbls = 0;
	while(state->tok.type == tok_ident && state->lookahead.type == tok_semi) {
		label *lbl = malloc(sizeof(label));
		lbl->ident = state->tok.ident;
		lbl->list.next = (list_head *) lbls;
		lbl->line = state->tok.line;
		lbl->column = state->tok.column;
		lbls = lbl;

		parser_next_twice(state);
		while(state->tok.type == tok_newline) { parser_next(state); }
		if(state->tok.type == tok_eof) {
			fprintf(stderr, "error: label references nothing at line %zu, column %zu\n",
			        state->tok.line, state->tok.column
			);
			state->iserr = true;
			list_head_destroy(lbls);
			return 0;
		}
	}
	return lbls;
}

/* 0 | a | a:b */
static arg *parser_arg(parser_state *state) {
	arg *new_arg = malloc(sizeof(arg));
	new_arg->line = state->tok.line;
	new_arg->column = state->tok.column;
	switch(state->tok.type) {
		case tok_number:
			new_arg->type = arg_number;
			new_arg->num = state->tok.num;
			break;
		case tok_ident:
			if(state->lookahead.type == tok_semi) {
				size_t ident1 = state->tok.ident;
				if(parser_next_twice(state) != tok_ident) {
					fprintf(stderr, "error: expected register name at line %zu, column %zu\n",
						state->tok.line, state->tok.column
					);
					state->iserr = true;
					free(new_arg);
					return 0;
				}
				new_arg->type = arg_pair;
				new_arg->pair.ident1 = ident1;
				new_arg->pair.ident2 = state->tok.ident;
			} else {
				new_arg->type = arg_ident;
				new_arg->ident = state->tok.ident;
			}
			break;
		default:
			free(new_arg);
			return 0;
	}
	parser_next(state);
	return new_arg;
}

/* arg | arglist ',' arg */
static arg *parser_arglist(parser_state *state) {
	arg *first_arg = parser_arg(state);

	arg *cur_arg = first_arg;
	arg *next_arg;
	while(state->tok.type == tok_comma) {
		parser_next(state);
		next_arg = parser_arg(state);
		if(!next_arg) {
			fprintf(stderr, "error: expected argument at line %zu, column %zu\n",
				state->tok.line, state->tok.column
			);
			list_head_destroy(first_arg);
			parser_recover(state);
			return 0;
		}
		cur_arg->list.next = (list_head*) next_arg;
		cur_arg = next_arg;
	}
	return first_arg;
}

/* labellist mov arglist [;comment] '\n' */
static void parser_ident(parser_state *state) {
	label *lbls = parser_label_list(state);
	if(state->tok.type != tok_ident) {
		fprintf(stderr, "error: expected identifier at line %zu, column %zu\n",
			state->tok.line, state->tok.column
		);
		list_head_destroy(lbls);
		parser_recover(state);
		return;
	}
	size_t op = state->tok.ident;
	size_t op_line = state->tok.line;
	size_t op_column = state->tok.column;
	parser_next(state);
	arg* arglist = 0;
	if(state->tok.type != tok_newline) arglist = parser_arglist(state);
	if(state->tok.type != tok_newline && state->tok.type != tok_eof) {
		fprintf(stderr, "error: expected new line at line %zu, column %zu\n",
			state->tok.line, state->tok.column
		);
		list_head_destroy(lbls);
		list_head_destroy(arglist);
		parser_recover(state);
		return;
	}
	parser_next(state);

	if(!state->iserr) {
		insn* new_insn = malloc(sizeof(insn));
		new_insn->lbls = lbls;
		new_insn->op = op;
		new_insn->args = arglist;
		new_insn->line = op_line;
		new_insn->column = op_column;
		state->insns_tail->list.next = (list_head*) new_insn;
		state->insns_tail = new_insn;
		new_insn->dirs = state->dirs;
		state->dirs = 0;
	}
}

/* .code */
static void parser_directive(parser_state *state) {
	if(state->tok.ident == state->kw_dir_org) {
		if(state->lookahead.type != tok_number) {
			fprintf(stderr, "error: expected address after .org directive at line %zu, column %zu\n",
			        state->lookahead.line, state->lookahead.column
			);
			state->iserr = true;
			parser_recover(state);
			return;
		}
		uint64_t addr = state->lookahead.num;
		parser_next(state);

		directive *dir = malloc(sizeof(directive));
		dir->dir = DIR_ORG;
		dir->num = addr;
		dir->list.next = (list_head *) state->dirs;
		state->dirs = dir;
	} else {
		fprintf(stderr, "error: unrecognized directive: %s\n",
		        symtbl_get(state->lexer->symtbl, state->tok.ident)
		);
		parser_recover(state);
		return;
	}
	parser_next(state);
}

static const char *token_to_string(enum token_type type) {
	switch(type) {
		case tok_eof:    return "tok_eof";

		case tok_ident:  return "tok_ident";
		case tok_number: return "tok_number";
		case tok_directive: return "tok_directive";

		case tok_semi:   return "tok_semi";
		case tok_comma:  return "tok_comma";
		case tok_newline: return "tok_newline";
	}
	return "tok_undefined";
}

void parser_parse(parser_state *state) {
	parser_next_twice(state);
	while(1) {
		if(state->isfailed) {
			state->iserr = true;
			return;
		}
		enum token_type type = state->tok.type;
		switch(type) {
			case tok_eof:
				return;
			case tok_ident:
				parser_ident(state);
				break;
			case tok_directive:
				parser_directive(state);
				break;
			case tok_newline:
				parser_next(state);
				break;
			default:
				fprintf(stderr, "error: unextected token: %s, at line %zu, column %zu\n",
				        token_to_string(type), state->tok.line, state->tok.column
				);
				parser_recover(state);
				break;
		}
	}
}

insn *parser_get(parser_state *parser) {
	return (insn *) parser->insns_head->list.next;
}

void parser_end(parser_state *state) {
	lexer_end(state->lexer);
	insn *in = state->insns_head;
	while(in) {
		insn *t = in;
		in = (insn *) in->list.next;
		list_head_destroy(t->dirs);
		list_head_destroy(t->lbls);
		list_head_destroy(t->args);
		free(t);
	}
	free(state);
}
