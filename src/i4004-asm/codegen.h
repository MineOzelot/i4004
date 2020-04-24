#ifndef I4004_CODEGEN_H
#define I4004_CODEGEN_H

#include "parser.h"
#include "list_head.h"
#include "section.h"
#include "error.h"

enum e_symbol_type {
	SYM_COMMON
};

//TODO: hashtable
typedef struct {
	list_head list;

	size_t ident;
	size_t offset;
	enum e_symbol_type type;

	position pos;
} symbol;

symbol *symbol_find(symbol *symbols, size_t ident);

enum e_reference_type {
	REF_FIRST_HALF,             // DDDDxxxx
	REF_LAST_HALF,              // xxxxDDDD

	REF_LAST_HALF_2HALF,        // xxxxCCCC AAAABBBB

	REF_BYTE,                   // AAAABBBB
};

typedef struct {
	list_head list;

	size_t ident;
	size_t offset;
	enum e_reference_type type;

	position pos;
} reference;

enum e_resolve {
	RESOLVE_NULL = 0x0000,

	RESOLVE_REFERENCE = 0xFFFE,
	RESOLVE_ERROR = 0xFFFF
};

typedef struct {
	symtbl *tbl;

	section *sect;

	symbol *symbols;
	reference *references;

	size_t cur_page;

	struct {
		size_t opcodes[OP_ESIZE];
		size_t jcn_aliases[JCN_ESIZE];
		size_t pseudos[PSEUDO_ESIZE];
		size_t registers[R_ESIZE];
	} idents;

	bool iserr;
} codegen_state;

codegen_state *codegen_from_insnlist(insn *insnlist, symtbl *tbl);
void codegen_destroy(codegen_state *state);

#endif //I4004_CODEGEN_H
