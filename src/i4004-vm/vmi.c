#include <unistd.h>
#include "vmi.h"

static void vmi_sleep(vm_state *vm) {
	uint32_t delay = ram_read_dword(vm->ram[0][3], 3, 0);
	usleep(delay);
}

static void state_ram_write_status(ram_chip *ram, uint8_t reg, uint8_t idx, uint8_t data) {
	if(reg == 3) {
		if(idx == 0) {
			if(data == VM_STATE_SLEEPING) {
				vmi_sleep(ram->vm);
				ram_write_status(ram, 3, 0, VM_STATE_ACTIVE);
			}
		}
	}
}

void vmi_register_ram_chips(vm_state *vm) {
	ram_chip *state_ram = vm->ram[0][3] = ram_create(vm);
	state_ram->interface.write_status = &state_ram_write_status;
}

void vmi_set_state(vm_state *vm, enum e_vm_state state) {
	ram_write_status(vm->ram[0][3], 3, 0, state);
}