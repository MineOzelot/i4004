#ifndef I4004_SECTION_H
#define I4004_SECTION_H

#include <stdint.h>
#include <stdbool.h>
#include "list_head.h"

typedef struct {
	list_head list;

	size_t offset;

	uint8_t *bytes;
	size_t len, size;
} fragment;

fragment *fragment_create(size_t offset);
size_t fragment_append(fragment *frag, uint8_t byte);
void fragment_destroy(fragment *frag);

typedef struct {
	fragment *frag;

	fragment *current;
} section;

section *section_create();
fragment *section_create_offset(section *sect, size_t offset);
fragment *section_create_current(section *sect, size_t offset);
bool section_verify(section *sect);
size_t section_calc_size(section *sect);
void section_destroy(section *sect);


#endif //I4004_SECTION_H
