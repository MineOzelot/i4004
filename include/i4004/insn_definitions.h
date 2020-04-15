#ifndef I4004_INSN_DEFINITIONS_H
#define I4004_INSN_DEFINITIONS_H

#include <stdint.h>
#include "enums.h"

typedef struct {
	const char *name;
	enum e_opcode e_op;
	uint8_t opcode;
	enum e_insn_arg_type e_args;
} insn_def;

typedef struct {
	const char *name;
	enum e_jcn_alias e_alias;
	uint8_t cond;
} jcn_alias_def;

typedef struct {
	const char *name;
	enum e_insn_pseudo e_pseudo;
} insn_pseudo_def;

typedef struct {
	const char *name;
	enum e_index_register e_reg;
} index_reg_def;

const insn_def instruction_defs[] = {
		{.name = "nop", .e_op = OP_NOP, .opcode = 0x00, .e_args = ARG_TYPE_NONE},
		{.name = "jcn", .e_op = OP_JCN, .opcode = 0x10, .e_args = ARG_TYPE_1COND_2ADDR},
		{.name = "fim", .e_op = OP_FIM, .opcode = 0x20, .e_args = ARG_TYPE_1PAIR_2DATA},
		{.name = "src", .e_op = OP_SRC, .opcode = 0x21, .e_args = ARG_TYPE_1PAIR},
		{.name = "fin", .e_op = OP_FIN, .opcode = 0x30, .e_args = ARG_TYPE_1PAIR},
		{.name = "jin", .e_op = OP_JIN, .opcode = 0x31, .e_args = ARG_TYPE_1PAIR},
		{.name = "jun", .e_op = OP_JUN, .opcode = 0x40, .e_args = ARG_TYPE_3ADDR},
		{.name = "jms", .e_op = OP_JMS, .opcode = 0x50, .e_args = ARG_TYPE_3ADDR},
		{.name = "inc", .e_op = OP_INC, .opcode = 0x60, .e_args = ARG_TYPE_1REG},
		{.name = "isz", .e_op = OP_ISZ, .opcode = 0x70, .e_args = ARG_TYPE_1REG_2ADDR},
		{.name = "add", .e_op = OP_ADD, .opcode = 0x80, .e_args = ARG_TYPE_1REG},
		{.name = "sub", .e_op = OP_SUB, .opcode = 0x90, .e_args = ARG_TYPE_1REG},
		{.name =  "ld", .e_op =  OP_LD, .opcode = 0xA0, .e_args = ARG_TYPE_1REG},
		{.name = "xch", .e_op = OP_XCH, .opcode = 0xB0, .e_args = ARG_TYPE_1REG},
		{.name = "bbl", .e_op = OP_BBL, .opcode = 0xC0, .e_args = ARG_TYPE_1DATA},
		{.name = "ldm", .e_op = OP_LDM, .opcode = 0xD0, .e_args = ARG_TYPE_1DATA},

		{.name = "wrm", .e_op = OP_WRM, .opcode = 0xE0, .e_args = ARG_TYPE_NONE},
		{.name = "wmp", .e_op = OP_WMP, .opcode = 0xE1, .e_args = ARG_TYPE_NONE},
		{.name = "wrr", .e_op = OP_WRR, .opcode = 0xE2, .e_args = ARG_TYPE_NONE},
		{.name = "wpm", .e_op = OP_WPM, .opcode = 0xE3, .e_args = ARG_TYPE_NONE},
		{.name = "wr0", .e_op = OP_WR0, .opcode = 0xE4, .e_args = ARG_TYPE_NONE},
		{.name = "wr1", .e_op = OP_WR1, .opcode = 0xE5, .e_args = ARG_TYPE_NONE},
		{.name = "wr2", .e_op = OP_WR2, .opcode = 0xE6, .e_args = ARG_TYPE_NONE},
		{.name = "wr3", .e_op = OP_WR3, .opcode = 0xE7, .e_args = ARG_TYPE_NONE},
		{.name = "sbm", .e_op = OP_SBM, .opcode = 0xE8, .e_args = ARG_TYPE_NONE},
		{.name = "rdm", .e_op = OP_RDM, .opcode = 0xE9, .e_args = ARG_TYPE_NONE},
		{.name = "rdr", .e_op = OP_RDR, .opcode = 0xEA, .e_args = ARG_TYPE_NONE},
		{.name = "adm", .e_op = OP_ADM, .opcode = 0xEB, .e_args = ARG_TYPE_NONE},
		{.name = "rd0", .e_op = OP_RD0, .opcode = 0xEC, .e_args = ARG_TYPE_NONE},
		{.name = "rd1", .e_op = OP_RD1, .opcode = 0xED, .e_args = ARG_TYPE_NONE},
		{.name = "rd2", .e_op = OP_RD2, .opcode = 0xEE, .e_args = ARG_TYPE_NONE},
		{.name = "rd3", .e_op = OP_RD3, .opcode = 0xEF, .e_args = ARG_TYPE_NONE},
		{.name = "clb", .e_op = OP_CLB, .opcode = 0xF0, .e_args = ARG_TYPE_NONE},
		{.name = "clc", .e_op = OP_CLC, .opcode = 0xF1, .e_args = ARG_TYPE_NONE},
		{.name = "iac", .e_op = OP_IAC, .opcode = 0xF2, .e_args = ARG_TYPE_NONE},
		{.name = "cmc", .e_op = OP_CMC, .opcode = 0xF3, .e_args = ARG_TYPE_NONE},
		{.name = "cma", .e_op = OP_CMA, .opcode = 0xF4, .e_args = ARG_TYPE_NONE},
		{.name = "ral", .e_op = OP_RAL, .opcode = 0xF5, .e_args = ARG_TYPE_NONE},
		{.name = "rar", .e_op = OP_RAR, .opcode = 0xF6, .e_args = ARG_TYPE_NONE},
		{.name = "tcc", .e_op = OP_TCC, .opcode = 0xF7, .e_args = ARG_TYPE_NONE},
		{.name = "dac", .e_op = OP_DAC, .opcode = 0xF8, .e_args = ARG_TYPE_NONE},
		{.name = "tcs", .e_op = OP_TCS, .opcode = 0xF9, .e_args = ARG_TYPE_NONE},
		{.name = "stc", .e_op = OP_STC, .opcode = 0xFA, .e_args = ARG_TYPE_NONE},
		{.name = "daa", .e_op = OP_DAA, .opcode = 0xFB, .e_args = ARG_TYPE_NONE},
		{.name = "kbp", .e_op = OP_KBP, .opcode = 0xFC, .e_args = ARG_TYPE_NONE},
		{.name = "dcl", .e_op = OP_DCL, .opcode = 0xFD, .e_args = ARG_TYPE_NONE},
		{.name = 0}

};

const jcn_alias_def jcn_alias_defs[] = {
		{.name = "jnt", .e_alias = JCN_JNT, .cond = 0b0001}, {.name =  "jc", .e_alias =  JCN_JC, .cond = 0b0010},
		{.name =  "jz", .e_alias =  JCN_JZ, .cond = 0b0100}, {.name =  "jt", .e_alias =  JCN_JT, .cond = 0b1001},
		{.name = "jnc", .e_alias = JCN_JNC, .cond = 0b1010}, {.name = "jnz", .e_alias = JCN_JNZ, .cond = 0b1100},
		{.name = 0}
};

const insn_pseudo_def pseudo_insn_defs[] = {
		{.name = "db", .e_pseudo = PSEUDO_DB},
		{.name = 0}
};

const index_reg_def index_reg_defs[] = {
		{.name =  "r0", .e_reg =  R0}, {.name =  "r1", .e_reg =  R1},
		{.name =  "r2", .e_reg =  R2}, {.name =  "r3", .e_reg =  R3},
		{.name =  "r4", .e_reg =  R4}, {.name =  "r5", .e_reg =  R5},
		{.name =  "r6", .e_reg =  R6}, {.name =  "r7", .e_reg =  R7},
		{.name =  "r8", .e_reg =  R8}, {.name =  "r9", .e_reg =  R9},
		{.name = "r10", .e_reg = R10}, {.name = "r11", .e_reg = R11},
		{.name = "r12", .e_reg = R12}, {.name = "r13", .e_reg = R13},
		{.name = "r14", .e_reg = R14}, {.name = "r15", .e_reg = R15},
		{.name = 0}
};

#endif //I4004_INSN_DEFINITIONS_H
