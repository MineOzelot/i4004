#include <stdlib.h>
#include <stdio.h>
#include "section.h"

#define FRAGMENT_DEF_SIZE 32

fragment *fragment_create(size_t offset) {
	fragment *frag = malloc(sizeof(fragment));

	frag->list.next = 0;
	frag->offset = offset;
	frag->bytes = malloc(FRAGMENT_DEF_SIZE);
	frag->size = FRAGMENT_DEF_SIZE;
	frag->len = 0;

	return frag;
}

size_t fragment_append(fragment *frag, uint8_t byte) {
	if(frag->len >= frag->size) {
		size_t new_size = (size_t) (frag->size * 1.8 + 1);
		frag->bytes = realloc(frag->bytes, new_size);
		frag->size = new_size;
	}

	frag->bytes[frag->len++] = byte;

	return frag->offset + frag->len - 1;
}

void fragment_destroy(fragment *frag) {
	free(frag->bytes);
	free(frag);
}

section *section_create() {
	section *sect = malloc(sizeof(section));

	sect->frag = fragment_create(0);
	sect->current = sect->frag;

	return sect;
}

fragment *section_create_offset(section *sect, size_t offset) {
	fragment *frag = sect->frag;
	fragment *prev = frag;
	while(frag) {
		if(frag->offset > offset) {
			fragment *new_frag = fragment_create(offset);
			prev->list.next = (list_head *) new_frag;
			new_frag->list.next = (list_head *) frag;
			return new_frag;
		}

		prev = frag;
		frag = (fragment *) frag->list.next;
	}

	fragment *new_frag = fragment_create(offset);
	prev->list.next = (list_head *) new_frag;

	return new_frag;
}

fragment *section_create_current(section *sect, size_t offset) {
	sect->current = section_create_offset(sect, offset);
	return sect->current;
}

bool section_verify(section *sect) {
	fragment *frag = (fragment *) sect->frag->list.next;
	fragment *last = sect->frag;
	bool err = false;
	size_t last_ending = sect->frag->offset + sect->frag->len;
	while(frag) {
		if(last_ending >= frag->offset) {
			fprintf(stderr, "error: fragment with offset 0x%zx overlaps fragment 0x%zx\n",
			        frag->offset, last->offset
			);
			err = true;
		}
		last = frag;
		frag = (fragment*) frag->list.next;
	}

	return !err;
}

size_t section_calc_size(section *sect) {
	fragment *frag = sect->frag;
	while(frag->list.next) frag = (fragment *) frag->list.next;
	return frag->offset + frag->len;
}

void section_destroy(section *sect) {
	fragment *frag = sect->frag;
	while(frag) {
		fragment *f = frag;
		frag = (fragment *) frag->list.next;
		fragment_destroy(f);
	}
	free(sect);
}