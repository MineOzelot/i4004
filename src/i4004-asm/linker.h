#include <i4004/enums.h>
#include "section.h"
#include "codegen.h"

#ifndef I4004_LINKER_H
#define I4004_LINKER_H

typedef struct {
	symtbl *tbl;

	uint8_t *sections[SEC_ESIZE];

	bool iserr;
} linker_state;

linker_state *linker_create(symtbl *tbl);

void linker_put_section(linker_state *state, section *sect, enum e_section type);

void linker_link(linker_state *state, symbol *symbols, reference *references);

void linker_destroy(linker_state *state);

#endif //I4004_LINKER_H
