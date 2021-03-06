#ifndef I4004_PARSER_H
#define I4004_PARSER_H

#include <i4004/enums.h>

#include "lexer.h"
#include "list_head.h"
#include "preproc.h"

typedef struct label label;
struct label {
	list_head list;

	size_t ident;

	position pos;
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

	position pos;
};

enum e_directive {
	DIR_ORG,
	DIR_PAGE,
	DIR_UNDEF
};

typedef struct directive directive;
struct directive {
	list_head list;

	enum e_directive dir;
	uint64_t num;
};

typedef struct insn insn;
struct insn {
	list_head list;

	directive *dirs;
	label *lbls;
	size_t op;
	arg *args;

	position pos;
};

typedef struct {
	preproc_state *lexer;

	token tok, lookahead;

	bool iserr;
	bool isfailed;

	insn *insns_head;
	insn *insns_tail;

	directive *dirs;
	directive *dirs_tail;

	size_t kw_dir_org, kw_dir_page;
} parser_state;

parser_state *parser_start(preproc_state *lexer);

void parser_parse(parser_state *parser);

insn *parser_get(parser_state *parser);

void parser_end(parser_state *parser);

#endif //I4004_PARSER_H
