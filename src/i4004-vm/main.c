#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <i4004/enums.h>
#include "vm.h"

int main(int argc, char *argv[]) {
	int opt;
	opterr = 0;

	const char *image_file = 0;

	while((opt = getopt(argc, argv, "")) != -1) {
		switch(opt) {
			case '?':
				fprintf(stderr, "error: unknown option character `\\x%x`\n", optopt);
				return 1;
			default:
				abort();
		}
	}

	if(optind >= argc) {
		fprintf(stderr, "error: no image file\n");
		return 1;
	}

	image_file = argv[optind];

	if(optind + 1 < argc) {
		fprintf(stderr, "error: too many image files\n");
		return 1;
	}

	int ret_val = 1;

	uint8_t *code_section = 0;
	uint8_t *data_section = 0;

	FILE *image = fopen(image_file, "rb");
	if(!image) {
		fprintf(stderr, "error: could not open image file\n");
		goto destroy;
	}

	code_section = malloc(SECTION_SIZE_MAX);
	size_t read = fread(code_section, 1, SECTION_SIZE_MAX, image);
	if(read != SECTION_SIZE_MAX) {
		fprintf(stderr, "error: could not read code section\n");
		goto destroy;
	}

	data_section = malloc(SECTION_SIZE_MAX);
	read = fread(data_section, 1, SECTION_SIZE_MAX, image);
	if(read != SECTION_SIZE_MAX) {
		fprintf(stderr, "error: could not read data section\n");
		goto destroy;
	}

	vm_state *vm = vm_create();
	vm_put_section(vm, code_section, SEC_CODE);
	vm_put_section(vm, data_section, SEC_DATA);

	while(!vm->is_terminated) vm_tick(vm);

	vm_destroy(vm);

	ret_val = 0;
destroy:
	if(data_section) free(data_section);
	if(code_section) free(code_section);
	if(image) fclose(image);
	return ret_val;
}