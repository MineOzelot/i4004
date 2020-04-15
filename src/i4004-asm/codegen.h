#ifndef I4004_CODEGEN_H
#define I4004_CODEGEN_H

#include "parser.h"
#include "list_head.h"
#include "section.h"

enum e_symbol_type {
	SYM_COMMON
};

//TODO: hashtable
typedef struct {
	list_head list;

	size_t ident;
	size_t offset;
	enum e_symbol_type type;
	enum e_section section;

	size_t line, column;
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
	enum e_section section;

	size_t line, column;
} reference;

enum e_resolve: uint16_t {
	RESOLVE_NULL = 0x0000,

	RESOLVE_REFERENCE = 0xFFFE,
	RESOLVE_ERROR = 0xFFFF
};

typedef struct {
	symtbl *tbl;

	section *sections[SEC_ESIZE];

	symbol *symbols;
	reference *references;

	struct {
		size_t opcodes[OP_ESIZE];
		size_t jcn_aliases[JCN_ESIZE];
		size_t pseudos[PSEUDO_ESIZE];
		size_t registers[R_ESIZE];
	} idents;

	bool iserr;
} codegen_state;

static inline uint8_t codegen_rearrange_byte(uint8_t byte) {
	return (uint8_t) (((byte << 4) & 0xF0) | ((byte >> 4) & 0x0F));
}

codegen_state *codegen_from_insnlist(insn *insnlist, symtbl *tbl);
void codegen_destroy(codegen_state *state);

#endif //I4004_CODEGEN_H
