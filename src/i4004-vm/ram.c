#include <malloc.h>
#include <memory.h>
#include "ram.h"

ram_chip *ram_create(struct vm_state *vm) {
	ram_chip *ram = malloc(sizeof(ram_chip));

	ram->vm = vm;
	memset(ram->data, 0, RAM_CHIP_BYTES);
	memset(ram->status, 0, RAM_CHIP_STATUS_BYTES);

	return ram;
}

void ram_write_half(ram_chip *ram, uint8_t reg, uint8_t half, uint8_t data) {
	uint8_t val = ram->data[(reg << 2) | (half >> 1)];
	if(half & 1)
		ram->data[(reg << 2) | (half >> 1)] = (uint8_t) ((val & 0xF0) | (data & 0x0F));
	else
		ram->data[(reg << 2) | (half >> 1)] = (uint8_t) ((val & 0x0F) | (data << 4));
}

uint8_t ram_read_half(ram_chip *ram, uint8_t reg, uint8_t half) {
	uint8_t val = ram->data[(reg << 2) | (half >> 1)];
	if(half & 1)
		return (uint8_t) (val & 0x0F);
	else
		return (uint8_t) ((val >> 4) & 0x0F);
}

void ram_write_status(ram_chip *ram, uint8_t reg, uint8_t idx, uint8_t data) {
	uint8_t val = ram->status[reg << 2 | idx >> 1];
	if(idx & 1)
		ram->status[(reg << 2) | (idx >> 1)] = (uint8_t) ((val & 0xF0) | (data & 0x0F));
	else
		ram->status[(reg << 2) | (idx >> 1)] = (uint8_t) ((val & 0x0F) | (data << 4));
}

uint8_t ram_read_status(ram_chip *ram, uint8_t reg, uint8_t idx) {
	uint8_t val = ram->status[reg << 2 | idx >> 1];
	if(idx & 1)
		return (uint8_t) (val & 0x0F);
	else
		return (uint8_t) ((val >> 4) & 0x0F);
}

void ram_write_out(ram_chip *ram, uint8_t half) {}

void ram_destroy(ram_chip *ram) {
	free(ram);
}