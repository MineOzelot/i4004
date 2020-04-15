#ifndef I4004_ENUMS_H
#define I4004_ENUMS_H

#define SECTION_SIZE_MAX 4096L

enum e_section {
	SEC_CODE, SEC_DATA,

	SEC_ESIZE
};

enum e_index_register {
	R0 = 0, R1,
	R2, R3,
	R4, R5,
	R6, R7,
	R8, R9,
	R10, R11,
	R12, R13,
	R14, R15,

	R_ESIZE
};

enum e__index_register_pair {
	R0_R1 = 0,
	R2_R3,
	R4_R5,
	R6_R7,
	R8_R9,
	R10_R11,
	R12_R13,
	R14_R15,

	R_PAIR_ESIZE
};

enum e_opcode {
	OP_NOP = 0,

	OP_JCN,
	OP_FIM, OP_SRC,
	OP_FIN, OP_JIN,
	OP_JUN, OP_JMS,
	OP_INC,
	OP_ISZ,
	OP_ADD, OP_SUB,
	OP_LD, OP_XCH,
	OP_BBL, OP_LDM,

	OP_WRM, OP_WMP, OP_WRR, OP_WPM,
	OP_WR0, OP_WR1, OP_WR2, OP_WR3,
	OP_SBM, OP_RDM, OP_RDR, OP_ADM,
	OP_RD0, OP_RD1, OP_RD2, OP_RD3,
	OP_CLB, OP_CLC, OP_IAC,
	OP_CMC, OP_CMA,
	OP_RAL, OP_RAR,
	OP_TCC, OP_DAC,
	OP_TCS, OP_STC,
	OP_DAA, OP_KBP, OP_DCL,

	OP_ESIZE
};

enum e_jcn_alias {
	JCN_JNT = 0,
	JCN_JC,
	JCN_JZ,
	JCN_JT,
	JCN_JNC,
	JCN_JNZ,

	JCN_ESIZE
};

enum e_insn_pseudo {
	PSEUDO_DB = 0,

	PSEUDO_ESIZE
};

enum e_insn_arg_type {
	ARG_TYPE_NONE,                // no operands
	ARG_TYPE_1COND_2ADDR,         // xxxxCCCC AAAAAAAA, special type for jcn
	ARG_TYPE_1PAIR_2DATA,         // xxxxRRR0 DDDDDDDD, special type for fim
	ARG_TYPE_1PAIR,               // xxxxRRRx,          like src
	ARG_TYPE_3ADDR,               // xxxxAAAA AAAAAAAA, like jun
	ARG_TYPE_1REG,                // xxxxRRRR,          like inc
	ARG_TYPE_1REG_2ADDR,          // xxxxRRRR AAAAAAAA, special type for isz
	ARG_TYPE_1DATA,               // xxxxDDDD,          like bbl
};

#endif //I4004_ENUMS_H
