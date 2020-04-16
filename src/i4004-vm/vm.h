#ifndef I4004_VM_H
#define I4004_VM_H

#include <stdint.h>
#include <i4004/enums.h>
#include <stdbool.h>

#define VM_RAM_BANK_SIZE 0x7F
#define VM_STATUS_RAM_SIZE 0x20

typedef struct {
	uint8_t accum;
	uint8_t carry;

	uint8_t in_regs[16];

	uint16_t pc;

	uint16_t stack[3];
} vm_register_box;

typedef struct {
	uint8_t a : 4;
	uint8_t b : 4;
} ram_cell;

typedef struct {
	uint8_t *section[SEC_ESIZE];

	vm_register_box regs;

	uint8_t memory_bank;
	uint8_t data_ptr;
	ram_cell *ram[8];
	ram_cell *status_ram[8];

	bool pin10;
	bool is_terminated;
} vm_state;

vm_state *vm_create();

void vm_put_section(vm_state *vm, uint8_t *section, enum e_section type);

void vm_tick(vm_state *vm);

void vm_destroy(vm_state *vm);

#endif //I4004_VM_H
