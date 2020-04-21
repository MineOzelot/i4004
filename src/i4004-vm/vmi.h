#ifndef I4004_VMI_H
#define I4004_VMI_H

#include "vm.h"

enum e_vm_state {
	VM_STATE_ACTIVE = 0,
	VM_STATE_TERMINATED = 1,
	VM_STATE_SLEEPING = 2
};

void vmi_register_ram_chips(vm_state *vm);

static inline enum e_vm_state vmi_get_state(vm_state *vm) {
	return (enum e_vm_state) (ram_read_status(vm->ram[0][3], 3, 0));
}

void vmi_set_state(vm_state *vm, enum e_vm_state state);

#endif //I4004_VMI_H
