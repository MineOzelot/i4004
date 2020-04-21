#ifndef I4004_ROM_H
#define I4004_ROM_H

#include <stdint.h>

typedef struct {
	struct vm_state *vm;
	uint8_t *page;
} rom_chip;

rom_chip *rom_create(struct vm_state *vm, uint8_t *image);

uint8_t rom_read_byte(rom_chip *rom, uint8_t addr);

void rom_write_io(rom_chip *rom, uint8_t half);
uint8_t rom_read_io(rom_chip *rom);

void rom_destroy(rom_chip *rom);

#endif //I4004_ROM_H
