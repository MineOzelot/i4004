#ifndef I4004_SYMTBL_H
#define I4004_SYMTBL_H

#include <stddef.h>

#include "string.h"

//TODO: hashtable
typedef struct {
	string **idents;
	size_t size, len;
} symtbl;

symtbl *symtbl_create();

size_t symtbl_ident(symtbl *tbl, string *str);

const char *symtbl_get(symtbl *tbl, size_t index);

void symtbl_destroy(symtbl *tbl);

#endif //I4004_SYMTBL_H