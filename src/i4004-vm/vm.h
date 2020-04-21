#ifndef I4004_VM_H
#define I4004_VM_H

#include <stdint.h>
#include <stdbool.h>
#include <i4004/enums.h>
#include "ram.h"
#include "rom.h"

typedef struct {
	uint8_t accum: 4;
	uint8_t carry: 1;

	uint8_t in_regs[16];

	uint16_t pc: 12;

	uint16_t stack[3];
} vm_register_box;

typedef struct {
	uint8_t chip_ptr: 2;
	uint8_t reg_ptr: 2;
	uint8_t half_ptr: 4;
} data_ptr;

typedef struct vm_state {
	vm_register_box regs;

	uint8_t memory_bank: 3;

	data_ptr dp;

	ram_chip *ram[8][4];
	rom_chip *rom[16];

	bool pin10;
} vm_state;

vm_state *vm_create();

void vm_put_section(vm_state *vm, uint8_t *section);

void vm_tick(vm_state *vm);
void vm_run(vm_state *vm);

void vm_destroy(vm_state *vm);

#endif //I4004_VM_H
