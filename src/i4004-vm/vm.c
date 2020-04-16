#include <malloc.h>
#include <memory.h>
#include <i4004/insn_definitions.h>
#include "vm.h"

vm_state *vm_create() {
	vm_state *vm = malloc(sizeof(vm_state));

	memset(vm, 0, sizeof(vm_state));

	return vm;
}

void vm_put_section(vm_state *vm, uint8_t *section, enum e_section type) {
	vm->section[type] = section;
}

static inline uint8_t vm_read_pc(vm_state *vm) {
	return vm->section[SEC_CODE][vm->regs.pc++];
}

static void vm_jump_byte(vm_state *vm, uint8_t addr) {
	uint16_t pc = (uint16_t) (vm->regs.pc & 0xF00);
	vm->regs.pc = pc | addr;
}

static void vm_jump_word(vm_state *vm, uint16_t addr) {
	vm->regs.pc = (uint16_t) (addr & 0xFFF);
}

static void vm_push_pc(vm_state *vm) {
	vm->regs.stack[2] = vm->regs.stack[1];
	vm->regs.stack[1] = vm->regs.stack[0];
	vm->regs.stack[0] = vm->regs.pc;
}

static void vm_pop_pc(vm_state *vm) {
	vm->regs.pc = vm->regs.stack[0];
	vm->regs.stack[0] = vm->regs.stack[1];
	vm->regs.stack[1] = vm->regs.stack[2];
	vm->regs.stack[2] = 0;
}

static ram_cell *vm_ram_get_bank(vm_state *vm) {
	if(!vm->ram[vm->memory_bank]) vm->ram[vm->memory_bank] = calloc(VM_RAM_BANK_SIZE, sizeof(ram_cell));
	return vm->ram[vm->memory_bank];
}

static void vm_ram_write(vm_state *vm, uint8_t half) {
	ram_cell *bank = vm_ram_get_bank(vm);
	if(vm->data_ptr & 1) {
		bank[vm->data_ptr >> 1].b = (uint8_t) (half & 0xF);
	} else {
		bank[vm->data_ptr >> 1].a = (uint8_t) (half & 0xF);
	}
}

static uint8_t vm_ram_read(vm_state *vm) {
	ram_cell *bank = vm_ram_get_bank(vm);
	if(vm->data_ptr & 1) {
		return bank[vm->data_ptr >> 1].b;
	} else {
		return bank[vm->data_ptr >> 1].a;
	}
}

static ram_cell *vm_status_ram_get_bank(vm_state *vm) {
	if(!vm->status_ram[vm->memory_bank]) vm->status_ram[vm->memory_bank] = calloc(VM_STATUS_RAM_SIZE, sizeof(ram_cell));
	return vm->status_ram[vm->memory_bank];
}

static void vm_status_ram_write(vm_state *vm, uint8_t inx, uint8_t half) {
	ram_cell *bank = vm_status_ram_get_bank(vm);
	size_t addr = (vm->data_ptr >> 4) * inx;
	if(inx & 1) {
		bank[addr >> 1].b = (uint8_t) (half & 0xF);
	} else {
		bank[addr >> 1].a = (uint8_t) (half & 0xF);
	}
}

static uint8_t vm_status_ram_read(vm_state *vm, uint8_t inx) {
	ram_cell *bank = vm_status_ram_get_bank(vm);
	size_t addr = (vm->data_ptr >> 4) * inx;
	if(inx & 1) {
		return bank[addr >> 1].b;
	} else {
		return bank[addr >> 1].a;
	}
}

static inline void vm_exec_jcn(vm_state *vm, uint8_t cond, uint8_t addr) {
	bool do_jump = false;
	if((cond & 0b0100) && vm->regs.accum == 0) do_jump = true;
	if((cond & 0b0010) && vm->regs.carry == 1) do_jump = true;
	if((cond & 0b0001) && vm->pin10 == 0) do_jump = true;
	if((cond & 0b1000)) do_jump = !do_jump;

	if(do_jump) {
		vm_jump_byte(vm, addr);
	}
}

static inline void vm_exec_fim(vm_state *vm, uint8_t pair, uint8_t data) {
	vm->regs.in_regs[pair]     = (uint8_t) ((data >> 4) & 0xF);
	vm->regs.in_regs[pair + 1] = (uint8_t) (data & 0xF);
}

static inline void vm_exec_src(vm_state *vm, uint8_t pair) {
	vm->data_ptr = (vm->regs.in_regs[pair] << 4) | vm->regs.in_regs[pair + 1];
}

static inline void vm_exec_fin(vm_state *vm, uint8_t pair) {
	uint8_t r0_r1 = (vm->regs.in_regs[0] << 4) | vm->regs.in_regs[1];
	uint16_t addr = (uint16_t) (vm->regs.pc & 0xF00) | r0_r1;
	uint8_t data = vm->section[SEC_DATA][addr];
	vm->regs.in_regs[pair] = (uint8_t) ((data >> 4) & 0xF);
	vm->regs.in_regs[pair + 1] = (uint8_t) (data & 0xF);
}

static inline void vm_exec_jin(vm_state *vm, uint8_t pair) {
	uint8_t r0_r1 = (vm->regs.in_regs[0] << 4) | vm->regs.in_regs[1];
	vm_jump_byte(vm, r0_r1);
}

static inline void vm_exec_jun(vm_state *vm, uint16_t addr) {
	vm_jump_word(vm, addr);
}

static inline void vm_exec_jms(vm_state *vm, uint16_t addr) {
	vm_push_pc(vm);
	vm_jump_word(vm, addr);
}

static inline void vm_exec_inc(vm_state *vm, uint8_t reg) {
	vm->regs.in_regs[reg] = (uint8_t) (vm->regs.in_regs[reg] & 0xF);
}

static inline void vm_exec_isz(vm_state *vm, uint8_t reg, uint8_t addr) {
	vm_exec_inc(vm, reg);
	if(vm->regs.in_regs[reg] != 0) vm_jump_byte(vm, addr);
}

static inline void vm_exec_add(vm_state *vm, uint8_t reg) {
	uint8_t result = vm->regs.in_regs[reg];
	result += vm->regs.accum + vm->regs.carry;
	vm->regs.accum = (uint8_t) (result & 0xF);
	if(result > 0xF) vm->regs.carry = 1;
	else vm->regs.carry = 0;
}

static inline void vm_exec_sub(vm_state *vm, uint8_t reg) {
	uint8_t result = ~(vm->regs.in_regs[reg]);
	uint8_t borrow = (uint8_t) (vm->regs.carry == 1 ? 0 : 1);
	result += vm->regs.accum + borrow;
	vm->regs.accum = (uint8_t) (result & 0xF);
	if(result > 0xF) vm->regs.carry = 0;
	else vm->regs.carry = 1;
}

static inline void vm_exec_ld(vm_state *vm, uint8_t reg) {
	vm->regs.accum = vm->regs.in_regs[reg];
}

static inline void vm_exec_xch(vm_state *vm, uint8_t reg) {
	uint8_t tmp = vm->regs.accum;
	vm->regs.accum = vm->regs.in_regs[reg];
	vm->regs.in_regs[reg] = tmp;
}

static inline void vm_exec_bbl(vm_state *vm, uint8_t data) {
	vm_pop_pc(vm);
	vm->regs.accum = data;
}

static inline void vm_exec_ldm(vm_state *vm, uint8_t data) {
	vm->regs.accum = data;
}

static inline void vm_exec_wrm(vm_state *vm) {
	vm_ram_write(vm, vm->regs.accum);
}

static inline void vm_exec_wmp(vm_state *vm) {
	//TODO: ram extension
}

static inline void vm_exec_wrr(vm_state *vm) {
	//TODO: IO
}

static inline void vm_exec_wpm(vm_state *vm) {
	//TODO: rw program
}

static inline void vm_exec_wr0(vm_state *vm) {
	vm_status_ram_write(vm, 0, vm->regs.accum);
}

static inline void vm_exec_wr1(vm_state *vm) {
	vm_status_ram_write(vm, 1, vm->regs.accum);
}

static inline void vm_exec_wr2(vm_state *vm) {
	vm_status_ram_write(vm, 2, vm->regs.accum);
}

static inline void vm_exec_wr3(vm_state *vm) {
	vm_status_ram_write(vm, 3, vm->regs.accum);
}

static inline void vm_exec_sbm(vm_state *vm) {
	uint8_t result = ~(vm_ram_read(vm));
	uint8_t borrow = (uint8_t) (vm->regs.carry == 1 ? 0 : 1);
	result += vm->regs.accum + borrow;
	vm->regs.accum = (uint8_t) (result & 0xF);
	if(result > 0xF) vm->regs.carry = 0;
	else vm->regs.carry = 1;
}

static inline void vm_exec_rdm(vm_state *vm) {
	vm->regs.accum = vm_ram_read(vm);
}

static inline void vm_exec_rdr(vm_state *vm) {
	//TODO: IO
}

static inline void vm_exec_adm(vm_state *vm) {
	uint8_t result = vm_ram_read(vm);
	result += vm->regs.accum + vm->regs.carry;
	vm->regs.accum = (uint8_t) (result & 0xF);
	if(result > 0xF) vm->regs.carry = 1;
	else vm->regs.carry = 0;
}

static inline void vm_exec_rd0(vm_state *vm) {
	vm->regs.accum = vm_status_ram_read(vm, 0);
}

static inline void vm_exec_rd1(vm_state *vm) {
	vm->regs.accum = vm_status_ram_read(vm, 1);
}

static inline void vm_exec_rd2(vm_state *vm) {
	vm->regs.accum = vm_status_ram_read(vm, 2);
}

static inline void vm_exec_rd3(vm_state *vm) {
	vm->regs.accum = vm_status_ram_read(vm, 3);
}

static inline void vm_exec_clb(vm_state *vm) {
	vm->regs.accum = 0;
	vm->regs.carry = 0;
}

static inline void vm_exec_clc(vm_state *vm) {
	vm->regs.carry = 0;
}

static inline void vm_exec_iac(vm_state *vm) {
	vm->regs.accum = (uint8_t) ((vm->regs.accum + 1) & 0xF);
	if(vm->regs.accum == 0) vm->regs.carry = 1;
	else vm->regs.carry = 0;
}

static inline void vm_exec_cmc(vm_state *vm) {
	if(vm->regs.carry == 1) vm->regs.carry = 0;
	else vm->regs.carry = 1;
}

static inline void vm_exec_cma(vm_state *vm) {
	vm->regs.accum = (uint8_t) ((~vm->regs.accum) & 0xF);
}

static inline void vm_exec_ral(vm_state *vm) {
	uint8_t left_bit = (uint8_t) (vm->regs.accum & 0x8);
	vm->regs.accum <<= 1;
	vm->regs.accum |= vm->regs.carry;
	vm->regs.carry = left_bit;
}

static inline void vm_exec_rar(vm_state *vm) {
	uint8_t right_bit = (uint8_t) (vm->regs.accum & 1);
	vm->regs.accum >>= 1;
	vm->regs.accum |= vm->regs.carry << 3;
	vm->regs.carry = right_bit;
}

static inline void vm_exec_tcc(vm_state *vm) {
	vm->regs.accum = vm->regs.carry;
	vm->regs.carry = 0;
}

static inline void vm_exec_dac(vm_state *vm) {
	vm->regs.accum = (uint8_t) ((vm->regs.accum - 1) & 0xF);
	if(vm->regs.accum == 0xF) vm->regs.carry = 0;
	else vm->regs.carry = 1;
}

static inline void vm_exec_tcs(vm_state *vm) {
	if(vm->regs.carry == 1) vm->regs.accum = 10;
	else vm->regs.accum = 9;
	vm->regs.carry = 0;
}

static inline void vm_exec_stc(vm_state *vm) {
	vm->regs.carry = 1;
}

static inline void vm_exec_daa(vm_state *vm) {
	uint8_t result = vm->regs.accum;
	if(vm->regs.carry == 1 || vm->regs.accum > 9) result += 6;
	vm->regs.accum = (uint8_t) (result & 0xF);
	if(result > 0xF) vm->regs.carry = 1;
}

static const uint8_t kbp_table[] = {
		0b0000, 0b0001, 0b0010, 0b0011, 0b0100, 0b1111, 0b1111, 0b1111,
		0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111
};

static inline void vm_exec_kbp(vm_state *vm) {
	vm->regs.accum = kbp_table[vm->regs.accum];
}

static inline void vm_exec_dcl(vm_state *vm) {
	vm->memory_bank = (uint8_t) (vm->regs.accum & 7);
}

static void vm_exec_insn(vm_state *vm, const insn_def *def, uint8_t op) {
	uint8_t byte1 = 0, byte2 = 0;
	uint16_t word = 0;
	switch(def->e_args) {
		case ARG_TYPE_NONE:
			break;
		case ARG_TYPE_1COND_2ADDR:
			byte1 = (uint8_t) (op & 0x0F);
			byte2 = vm_read_pc(vm);
			break;
		case ARG_TYPE_1PAIR_2DATA:
			byte1 = (uint8_t) (op & 0x0E);
			byte2 = vm_read_pc(vm);
			break;
		case ARG_TYPE_1PAIR:
			byte1 = (uint8_t) (op & 0x0E);
			break;
		case ARG_TYPE_3ADDR:
			word = op << 8;
			byte1 = vm_read_pc(vm);
			word |= byte1;
			break;
		case ARG_TYPE_1REG_2ADDR:
			byte1 = (uint8_t) (op & 0x0F);
			word = vm_read_pc(vm);
			break;
		case ARG_TYPE_1REG:
		case ARG_TYPE_1DATA:
			byte1 = (uint8_t) (op & 0x0F);
			break;
	}

	switch(def->e_op) {
		case OP_NOP: break;
		case OP_JCN: vm_exec_jcn(vm, byte1, byte2); break;
		case OP_FIM: vm_exec_fim(vm, byte1, byte2); break;
		case OP_SRC: vm_exec_src(vm, byte1); break;
		case OP_FIN: vm_exec_fin(vm, byte1); break;
		case OP_JIN: vm_exec_jin(vm, byte1); break;
		case OP_JUN: vm_exec_jun(vm, word); break;
		case OP_JMS: vm_exec_jms(vm, word); break;
		case OP_INC: vm_exec_inc(vm, byte1); break;
		case OP_ISZ: vm_exec_isz(vm, byte1, byte2); break;
		case OP_ADD: vm_exec_add(vm, byte1); break;
		case OP_SUB: vm_exec_sub(vm, byte1); break;
		case  OP_LD:  vm_exec_ld(vm, byte1); break;
		case OP_XCH: vm_exec_xch(vm, byte1); break;
		case OP_BBL: vm_exec_bbl(vm, byte1); break;
		case OP_LDM: vm_exec_ldm(vm, byte1); break;

		case OP_WRM: vm_exec_wrm(vm); break;
		case OP_WMP: vm_exec_wmp(vm); break;
		case OP_WRR: vm_exec_wrr(vm); break;
		case OP_WPM: vm_exec_wpm(vm); break;
		case OP_WR0: vm_exec_wr0(vm); break;
		case OP_WR1: vm_exec_wr1(vm); break;
		case OP_WR2: vm_exec_wr2(vm); break;
		case OP_WR3: vm_exec_wr3(vm); break;
		case OP_SBM: vm_exec_sbm(vm); break;
		case OP_RDM: vm_exec_rdm(vm); break;
		case OP_RDR: vm_exec_rdr(vm); break;
		case OP_ADM: vm_exec_adm(vm); break;
		case OP_RD0: vm_exec_rd0(vm); break;
		case OP_RD1: vm_exec_rd1(vm); break;
		case OP_RD2: vm_exec_rd2(vm); break;
		case OP_RD3: vm_exec_rd3(vm); break;
		case OP_CLB: vm_exec_clb(vm); break;
		case OP_CLC: vm_exec_clc(vm); break;
		case OP_IAC: vm_exec_iac(vm); break;
		case OP_CMC: vm_exec_cmc(vm); break;
		case OP_CMA: vm_exec_cma(vm); break;
		case OP_RAL: vm_exec_ral(vm); break;
		case OP_RAR: vm_exec_rar(vm); break;
		case OP_TCC: vm_exec_tcc(vm); break;
		case OP_DAC: vm_exec_dac(vm); break;
		case OP_TCS: vm_exec_tcs(vm); break;
		case OP_STC: vm_exec_stc(vm); break;
		case OP_DAA: vm_exec_daa(vm); break;
		case OP_KBP: vm_exec_kbp(vm); break;
		case OP_DCL: vm_exec_dcl(vm); break;
	}
}

void vm_tick(vm_state *vm) {
	uint8_t op = vm_read_pc(vm);
	enum e_opcode e_op = instruction_lookup[op];
	if(e_op == OP_ESIZE) {
		fprintf(stderr, "vm error: unsupported opcode: %X\n", op);
		vm->is_terminated = true;
		return;
	}
	const insn_def *def = &instruction_defs[e_op];
	vm_exec_insn(vm, def, op);
}

void vm_destroy(vm_state *vm) {
	free(vm);
}