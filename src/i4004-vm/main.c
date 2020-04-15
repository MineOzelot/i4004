#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

static uint8_t prog_buf[4096] = {0};

static uint16_t prog_counter = 0;
static uint16_t stack1, stack2, stack3;

static uint8_t flags = 0;

static uint8_t accum = 0;
static uint8_t index_regs[8] = {0};

uint8_t get_rom_byte() {
	return prog_buf[prog_counter];
}

void next_rom_byte() {
	prog_counter = (uint16_t) ((prog_counter + 1) & 0x0FFF);
}

uint8_t take_rom_byte() {
	uint8_t byte = get_rom_byte();
	next_rom_byte();
	return byte;
}

void write_index(int r, uint8_t val) {
	int idx = r >> 1;
	if(r % 2 == 0)
		index_regs[idx] = (uint8_t) ((index_regs[idx] & 0x0F) | (val << 4));
	else
		index_regs[idx] = (uint8_t) ((index_regs[idx] & 0xF0) | (val & 0x0F));
}

uint8_t read_index(int r) {
	int idx = r >> 1;
	if(r % 2 == 0)
		return (uint8_t) ((index_regs[idx] & 0xF0) >> 4);
	else
		return (uint8_t) (index_regs[idx] & 0x0F);
}

void tick() {
	uint8_t inst = take_rom_byte();
	if(inst == 0x00) return; //nop
	uint8_t half = (uint8_t) (inst & 0xF0);
	if(half == 0x10) { //jcn
		bool do_jump = false;
		if((inst & 0b0100) && accum == 0) do_jump = true;
		if((inst & 0b0010) && (flags & 0b1)) do_jump = true;
		if((inst & 0b0001)) do_jump = true;

		if(inst & 0b1000) do_jump = !do_jump;

		if(do_jump) {
			uint8_t addr = get_rom_byte();
			prog_counter &= 0x0F00;
			prog_counter |= addr;
		}
	} else if(half == 0x20) { //fim
		uint8_t reg = (uint8_t) ((inst & 0x0F) >> 1);
	}
}

int main(int argc, char *argv[]) {
	if(argc != 2) {
		printf("Using: i4004 <filename>\n");
		return 1;
	}

	FILE *file = fopen(argv[1], "rb");
	if(!file) {
		fprintf(stderr, "Could not open the file.\n");
		return 1;
	}
	fread(prog_buf, 1, sizeof(prog_buf), file);
	fclose(file);

	while(1) {
		tick();
	}

	return 0;
}