#include <stdint.h>

#include "symtbl.h"
#include "string.h"

#define SYMTBL_DEF_IDENTS 32

symtbl *symtbl_create() {
	symtbl *tbl = malloc(sizeof(symtbl));

	tbl->idents = calloc(SYMTBL_DEF_IDENTS, sizeof(string *));
	tbl->size = SYMTBL_DEF_IDENTS;
	tbl->len = 0;

	return tbl;
}

static size_t symtbl_find(symtbl *tbl, string *str) {
	for(size_t i = 0; i < tbl->len; i++) {
		if(string_equal(tbl->idents[i], str)) return i;
	}
	return SIZE_MAX;
}

static size_t symtbl_insert(symtbl *tbl, string *str) {
	if(tbl->len >= tbl->size) {
		size_t new_size = (size_t) (tbl->size * 1.8);
		tbl->idents = realloc(tbl->idents, sizeof(string *) * new_size);
		tbl->size = new_size;
	}
	tbl->idents[tbl->len++] = str;
	return tbl->len - 1;
}

size_t symtbl_ident(symtbl *tbl, string *str) {
	size_t index = symtbl_find(tbl, str);
	if(index != SIZE_MAX) return index;
	return symtbl_insert(tbl, str);
}

const char *symtbl_get(symtbl *tbl, size_t index) {
	return tbl->idents[index]->data;
}

void symtbl_destroy(symtbl *tbl) {
	for(size_t i = 0; i < tbl->len; i++) string_destroy(tbl->idents[i]);
	free(tbl->idents);
	free(tbl);
}
