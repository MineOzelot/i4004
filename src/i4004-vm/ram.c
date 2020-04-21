#include <malloc.h>
#include <memory.h>
#include "ram.h"

ram_chip *ram_create(struct vm_state *vm) {
	ram_chip *ram = malloc(sizeof(ram_chip));

	ram->vm = vm;
	memset(ram->data, 0, RAM_CHIP_BYTES);
	memset(ram->status, 0, RAM_CHIP_STATUS_BYTES);

	ram->interface.write_half = 0;
	ram->interface.read_half = 0;
	ram->interface.on_read_half = 0;
	ram->interface.write_status = 0;
	ram->interface.read_status = 0;
	ram->interface.on_read_status = 0;
	ram->interface.write_out = 0;

	return ram;
}

void ram_write_half(ram_chip *ram, uint8_t reg, uint8_t half, uint8_t data) {
	uint8_t val = ram->data[(reg << 2) | (half >> 1)];
	if(half & 1)
		ram->data[(reg << 2) | (half >> 1)] = (uint8_t) ((val & 0xF0) | (data & 0x0F));
	else
		ram->data[(reg << 2) | (half >> 1)] = (uint8_t) ((val & 0x0F) | (data << 4));
	if(ram->interface.write_half) ram->interface.write_half(ram, reg, half, data);
}

uint8_t ram_read_half(ram_chip *ram, uint8_t reg, uint8_t half) {
	if(ram->interface.on_read_half) ram->interface.on_read_half(ram, reg, half);
	if(ram->interface.read_half) return ram->interface.read_half(ram, reg, half);
	uint8_t val = ram->data[(reg << 2) | (half >> 1)];
	if(half & 1)
		return (uint8_t) (val & 0x0F);
	else
		return (uint8_t) ((val >> 4) & 0x0F);
}

void ram_write(ram_chip *ram, uint8_t reg, uint8_t half, uint64_t data, int size) {
	for(int i = 0; i < size; i++) {
		ram_write_half(ram, reg, (uint8_t) (half + i), (uint8_t) (data & 0xF));
		data >>= 4;
	}
}

uint64_t ram_read(ram_chip *ram, uint8_t reg, uint8_t half, int size) {
	uint64_t data = 0;
	for(int i = size - 1; i >= 0; i--) {
		data <<= 4;
		data |= ram_read_half(ram, reg, (uint8_t) (half + i));
	}
	return data;
}

void ram_write_status(ram_chip *ram, uint8_t reg, uint8_t idx, uint8_t data) {
	uint8_t val = ram->status[reg << 2 | idx >> 1];
	if(idx & 1)
		ram->status[(reg << 2) | (idx >> 1)] = (uint8_t) ((val & 0xF0) | (data & 0x0F));
	else
		ram->status[(reg << 2) | (idx >> 1)] = (uint8_t) ((val & 0x0F) | (data << 4));
	if(ram->interface.write_status) ram->interface.write_status(ram, reg, idx, data);
}

uint8_t ram_read_status(ram_chip *ram, uint8_t reg, uint8_t idx) {
	if(ram->interface.on_read_status) ram->interface.on_read_status(ram, reg, idx);
	if(ram->interface.read_status) return ram->interface.read_status(ram, reg, idx);
	uint8_t val = ram->status[reg << 2 | idx >> 1];
	if(idx & 1)
		return (uint8_t) (val & 0x0F);
	else
		return (uint8_t) ((val >> 4) & 0x0F);
}

void ram_destroy(ram_chip *ram) {
	free(ram);
}