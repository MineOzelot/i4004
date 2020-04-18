#ifndef I4004_VM_H
#define I4004_VM_H

#include <stdint.h>
#include <stdbool.h>
#include <i4004/enums.h>

#define VM_RAM_BANK_SIZE 0xFF
#define VM_STATUS_RAM_SIZE 0x40

typedef struct {
	uint8_t accum;
	uint8_t carry;

	uint8_t in_regs[16];

	uint16_t pc;

	uint16_t stack[3];
} vm_register_box;

typedef struct {
	uint8_t *sect;

	vm_register_box regs;

	uint8_t memory_bank;
	uint8_t data_ptr;
	uint8_t *ram[8];
	uint8_t *status_ram[8];

	bool pin10;
} vm_state;

vm_state *vm_create();

void vm_put_section(vm_state *vm, uint8_t *section);

void vm_tick(vm_state *vm);
void vm_run(vm_state *vm);

void vm_destroy(vm_state *vm);

#endif //I4004_VM_H
