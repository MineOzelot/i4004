#include "parser.h"

parser_state *parser_start(preproc_state *lexer) {
	parser_state *state = malloc(sizeof(parser_state));

	state->lexer = lexer;
	state->iserr = false;
	state->isfailed = false;

	state->insns_head = state->insns_tail = malloc(sizeof(insn));

	state->dirs = state->dirs_tail = 0;

	state->kw_dir_org = symtbl_ident(lexer->tbl, string_from("org", 3));
	state->kw_dir_page = symtbl_ident(lexer->tbl, string_from("page", 4));

	return state;
}

static enum token_type parser_next(parser_state *state) {
	state->tok = state->lookahead;
	state->lookahead = preproc_token(state->lexer);
	if(state->lexer->iserr) state->isfailed = true;
	return state->tok.type;
}

static enum token_type parser_next_twice(parser_state *state) {
	state->tok = preproc_token(state->lexer);
	state->lookahead = preproc_token(state->lexer);
	if(state->lexer->iserr) state->isfailed = true;
	return state->tok.type;
}

static void parser_recover(parser_state *state) {
	state->iserr = true;
	while(parser_next(state) != TOK_NEWLINE && state->tok.type != TOK_EOF);
}

/* [label:]* */
static label *parser_label_list(parser_state *state) {
	label *lbls = 0;
	while(state->tok.type == TOK_IDENT && state->lookahead.type == ':') {
		label *lbl = malloc(sizeof(label));
		lbl->ident = state->tok.ident;
		lbl->list.next = (list_head *) lbls;
		lbl->pos = state->tok.pos;
		lbls = lbl;

		parser_next_twice(state);
		while(state->tok.type == TOK_NEWLINE) { parser_next(state); }
		if(state->tok.type == TOK_EOF) {
			position_error(state->tok.pos, "label references nothing\n");
			state->iserr = true;
			list_head_destroy(lbls);
			return 0;
		}
	}
	return lbls;
}

/* 0 | a | a:b | "abc" */
static arg *parser_arg(parser_state *state, arg *prev) {
	arg *new_arg = malloc(sizeof(arg));
	new_arg->pos = state->tok.pos;
	new_arg->list.next = 0;
	switch(state->tok.type) {
		case TOK_NUMBER:
			new_arg->type = arg_number;
			new_arg->num = state->tok.num;
			break;
		case TOK_IDENT:
			if(state->lookahead.type == ':') {
				size_t ident1 = state->tok.ident;
				if(parser_next_twice(state) != TOK_IDENT) {
					position_error(state->tok.pos, "expected register name\n");
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
		case TOK_STRING: {
			string *str = string_unesaped(state->lexer->tbl->idents[state->tok.ident]);
			arg *cur = new_arg;
			for(size_t i = 0; i < str->len; i++) {
				arg *ch = malloc(sizeof(arg));
				ch->pos = state->tok.pos;
				ch->list.next = 0;
				ch->type = arg_number;
				ch->num = (uint64_t) str->data[i];
				cur->list.next = (list_head *) ch;
				cur = ch;
			}
			arg *na = new_arg;
			new_arg = (arg *) new_arg->list.next;
			free(na);
			string_destroy(str);
			break;
		}
		default:
			free(new_arg);
			return 0;
	}
	parser_next(state);
	if(prev) prev->list.next = (list_head *) new_arg;
	return new_arg;
}

/* arg | arglist ',' arg */
static arg *parser_arglist(parser_state *state) {
	arg *first_arg = parser_arg(state, 0);

	arg *cur_arg = first_arg;
	arg *next_arg;
	while(state->tok.type == ',') {
		parser_next(state);
		next_arg = parser_arg(state, cur_arg);
		if(!next_arg) {
			position_error(state->tok.pos, "expected argument\n");
			list_head_destroy(first_arg);
			parser_recover(state);
			return 0;
		}
		cur_arg = next_arg;
	}
	return first_arg;
}

/* labellist mov arglist [;comment] '\n' */
static void parser_ident(parser_state *state) {
	label *lbls = parser_label_list(state);
	if(state->tok.type != TOK_IDENT) {
		position_error(state->tok.pos, "expected identifier\n");
		list_head_destroy(lbls);
		parser_recover(state);
		return;
	}
	size_t op = state->tok.ident;
	position op_pos = state->tok.pos;
	parser_next(state);
	arg* arglist = 0;
	if(state->tok.type != TOK_NEWLINE) arglist = parser_arglist(state);
	if(state->tok.type != TOK_NEWLINE && state->tok.type != TOK_EOF) {
		position_error(state->tok.pos, "expected new line\n");
		list_head_destroy(lbls);
		list_head_destroy(arglist);
		parser_recover(state);
		return;
	}
	parser_next(state);

	if(!state->iserr) {
		insn* new_insn = malloc(sizeof(insn));
		new_insn->lbls = lbls;
		new_insn->list.next = 0;
		new_insn->op = op;
		new_insn->args = arglist;
		new_insn->pos = op_pos;
		state->insns_tail->list.next = (list_head *) new_insn;
		state->insns_tail = new_insn;
		new_insn->dirs = state->dirs;
		state->dirs = 0;
	}
}

/* .code */
static void parser_directive(parser_state *state) {
	enum e_directive e_dir = DIR_UNDEF;
	if(state->tok.ident == state->kw_dir_org) e_dir = DIR_ORG;
	else if(state->tok.ident == state->kw_dir_page) e_dir = DIR_PAGE;

	if(e_dir == DIR_ORG || e_dir == DIR_PAGE) {
		if(state->lookahead.type != TOK_NUMBER) {
			position_error(state->lookahead.pos, "expected address after %s directive\n",
			               symtbl_get(state->lexer->tbl, state->tok.ident)
			);
			state->iserr = true;
			parser_recover(state);
			return;
		}
		uint64_t addr = state->lookahead.num;
		parser_next(state);

		directive *dir = malloc(sizeof(directive));
		dir->dir = e_dir;
		dir->num = addr;
		dir->list.next = 0;
		if(state->dirs_tail) {
			state->dirs_tail->list.next = (list_head *) dir;
			state->dirs_tail = dir;
		} else {
			state->dirs = state->dirs_tail = dir;
		}
	} else {
		position_error(state->tok.pos, "unrecognized directive: %s\n",
		               symtbl_get(state->lexer->tbl, state->tok.ident)
		);
		parser_recover(state);
		return;
	}
	parser_next(state);
}

static const char *token_to_string(enum token_type type) {
	switch(type) {
		case TOK_EOF:    return "tok_eof";

		case TOK_IDENT:  return "tok_ident";
		case TOK_NUMBER: return "tok_number";
		case TOK_DIRECTIVE: return "tok_directive";

		case TOK_NEWLINE: return "tok_newline";

		default:break;
	}
	if(type > 0 && type < 256) return "tok_punctuation";
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
			case TOK_EOF:
				return;
			case TOK_IDENT:
				parser_ident(state);
				break;
			case TOK_DIRECTIVE:
				parser_directive(state);
				break;
			case TOK_NEWLINE:
				parser_next(state);
				break;
			default:
				position_error(state->tok.pos, "unexpected token: %s\n", token_to_string(type));
				parser_recover(state);
				break;
		}
	}
}

insn *parser_get(parser_state *parser) {
	return (insn *) parser->insns_head->list.next;
}

void parser_end(parser_state *state) {
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
