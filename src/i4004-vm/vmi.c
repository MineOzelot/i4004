#include <unistd.h>
#include <stdio.h>
#include "vmi.h"

static void vmi_sleep(vm_state *vm) {
	uint32_t delay = ram_read_dword(vm->ram[0][3], 3, 0);
	usleep(delay);
}

static void state_ram_write_status(ram_chip *ram, uint8_t reg, uint8_t idx, uint8_t data) {
	uint8_t ch = 0;
	if(reg == 0) {
		switch(idx) {
			case 0:
				printf("%c", ram_read_byte(ram, 0, 0));
				break;
			case 1:
				scanf("%c", &ch);
				ram_write_byte(ram, 0, 2, ch);
				break;
			case 2:
				if(ram->vm->out_file) {
					ch = ram_read_byte(ram, 0, 4);
					fwrite(&ch, 1, 1, ram->vm->out_file);
				}
				break;
			case 3:
				if(ram->vm->in_file) {
					size_t read = fread(&ch, 1, 1, ram->vm->in_file);
					if(read < 0) ch = 0;
				}
				ram_write_byte(ram, 0, 6, ch);
				break;
			default: break;
		}
	} else if(reg == 3) {
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