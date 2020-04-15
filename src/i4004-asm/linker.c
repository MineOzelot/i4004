#include <memory.h>
#include "linker.h"

linker_state *linker_create(symtbl *tbl) {
	linker_state *state = malloc(sizeof(linker_state));

	state->tbl = tbl;
	state->iserr = false;

	for(int i = 0; i < SEC_ESIZE; i++) state->sections[i] = 0;

	return state;
}

static const char *e_section_name(enum e_section e_sect) {
	switch(e_sect) {
		case SEC_CODE: return "code";
		case SEC_DATA: return "data";
		default: return "undefined";
	}
}

void linker_put_section(linker_state *state, section *sect, enum e_section type) {
	if(!section_verify(sect)) {
		fprintf(stderr, "build terminated due to error in `%s` section\n", e_section_name(type));
		state->iserr = true;
		return;
	}

	size_t section_size = section_calc_size(sect);
	if(section_size > SECTION_SIZE_MAX) {
		fprintf(stderr, "error: section `%s` is too big (%zu > %zu)\n",
			e_section_name(type), section_size, SECTION_SIZE_MAX
		);
		state->iserr = true;
		return;
	}

	state->sections[type] = calloc(SECTION_SIZE_MAX, 1);

	fragment *frag = sect->frag;
	while(frag) {
		memcpy(state->sections[type] + frag->offset, frag->bytes, frag->len);

		frag = (fragment *) frag->list.next;
	}
}

static void linker_insert_reference(linker_state *state, symbol *sym, reference *ref) {
	uint8_t *sect = state->sections[ref->section];
	size_t sym_offset = sym->offset;
	size_t ref_offset = ref->offset;

	uint8_t byte;
	uint8_t written = sect[ref_offset];
	switch(ref->type) {
		case REF_FIRST_HALF:
			byte = (uint8_t) ((sym_offset << 4) & 0xF0);
			byte = (uint8_t) ((written & 0x0F) | byte);
			sect[ref_offset] = byte;
			break;
		case REF_LAST_HALF:
			byte = (uint8_t) ((sym_offset) & 0x0F);
			byte = (uint8_t) ((written & 0xF0) | byte);
			sect[ref_offset] = byte;
			break;
		case REF_LAST_HALF_2HALF:
			byte = (uint8_t) ((sym_offset >> 8) & 0x0F);
			byte = (uint8_t) ((written & 0xF0) | byte);
			sect[ref_offset] = byte;
			byte = (uint8_t) (sym_offset & 0xFF);
			sect[ref_offset + 1] = byte;
			break;
		case REF_BYTE:
			byte = (uint8_t) (sym_offset & 0xFF);
			sect[ref_offset] = byte;
			break;
	}
}

void linker_link(linker_state *state, symbol *symbols, reference *references) {
	reference *ref = references;
	while(ref) {
		symbol *sym = symbol_find(symbols, ref->ident);
		if(!sym) {
			fprintf(stderr, "error: undefined reference to `%s` at line %zu, column %zu\n",
				symtbl_get(state->tbl, ref->ident), ref->line, ref->column
			);
			state->iserr = true;
		} else {
			/*if(sym->section != ref->section) {
				fprintf(stderr,
				        "error: symbol and reference are in different sections: "
						"symbol `%s` at line %zu, column %zu, and reference at line %zu, column %zu\n",
				        symtbl_get(state->tbl, sym->ident), sym->line, sym->column,
				        ref->line, ref->column
				);
			}*/
			linker_insert_reference(state, sym, ref);
		}

		ref = (reference *) ref->list.next;
	}
}

void linker_destroy(linker_state *state) {
	free(state);
}