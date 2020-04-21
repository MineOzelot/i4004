#ifndef I4004_RAM_H
#define I4004_RAM_H

#include <stdint.h>

#define RAM_CHIP_BYTES 32
#define RAM_CHIP_STATUS_BYTES 8

typedef struct {
	struct vm_state *vm;

	uint8_t data[RAM_CHIP_BYTES];
	uint8_t status[RAM_CHIP_STATUS_BYTES];
} ram_chip;

ram_chip *ram_create(struct vm_state *vm);

void ram_write_half(ram_chip *ram, uint8_t reg, uint8_t half, uint8_t data);
uint8_t ram_read_half(ram_chip *ram, uint8_t reg, uint8_t half);

void ram_write_status(ram_chip *ram, uint8_t reg, uint8_t idx, uint8_t data);
uint8_t ram_read_status(ram_chip *ram, uint8_t reg, uint8_t idx);

void ram_write_out(ram_chip *ram, uint8_t half);

void ram_destroy(ram_chip *ram);

#endif //I4004_RAM_H
