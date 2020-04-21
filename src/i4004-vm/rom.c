#include <malloc.h>
#include "rom.h"

rom_chip *rom_create(struct vm_state *vm, uint8_t *image) {
	rom_chip *rom = malloc(sizeof(rom_chip));

	rom->vm = vm;
	rom->page = image;

	return rom;
}

uint8_t rom_read_byte(rom_chip *rom, uint8_t addr) {
	return rom->page[addr];
}

void rom_write_io(rom_chip *rom, uint8_t half) {}
uint8_t rom_read_io(rom_chip *rom) { return 0; }

void rom_destroy(rom_chip *rom) {
	free(rom);
}