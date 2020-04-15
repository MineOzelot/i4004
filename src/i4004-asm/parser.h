#ifndef I4004_PARSER_H
#define I4004_PARSER_H

#include <i4004/enums.h>

#include "lexer.h"
#include "list_head.h"

typedef struct label label;
struct label {
	list_head list;

	size_t ident;

	size_t line, column;
};

enum arg_type {
	arg_ident,
	arg_number,
	arg_pair
};

typedef struct arg arg;
struct arg {
	list_head list;

	enum arg_type type;
	union {
		size_t ident;
		uint64_t num;
		struct {
			size_t ident1, ident2;
		} pair;
	};

	size_t line, column;
};

typedef struct insn insn;
struct insn {
	list_head list;

	enum e_section section;
	label *lbls;
	size_t op;
	arg *args;

	size_t line, column;
};

typedef struct {
	lexer_state *lexer;

	token tok, lookahead;

	bool iserr;
	bool isfailed;

	enum e_section cur_section;

	insn *insns_head;
	insn *insns_tail;

	size_t kw_dir_code, kw_dir_data;
} parser_state;

parser_state *parser_start(lexer_state *lexer);

void parser_parse(parser_state *parser);

insn *parser_get(parser_state *parser);

void parser_end(parser_state *parser);

#endif //I4004_PARSER_H
