#include <memory.h>

#include <i4004/enums.h>
#include <i4004/insn_definitions.h>

#include "codegen.h"
#include "parser.h"
#include "section.h"

enum insn_arg_type {
	at_half, at_byte, at_triple,
	at_reg, at_reg_pair
};

symbol *symbol_find(symbol *symbols, size_t ident) {
	symbol *sym = symbols;
	while(sym) {
		if(sym->ident == ident) return sym;
		sym = (symbol *) sym->list.next;
	}
	return 0;
}

static size_t codegen_write(codegen_state *state, uint8_t byte, position pos) {
	size_t offset = state->sect->current->offset + state->sect->current->len;
	size_t page_offset = state->cur_page << 8;
	if(offset < page_offset || offset > page_offset + 0xFF) {
		position_warning(pos, "instruction is out of current page (0x%zx)\n", state->cur_page);
		state->cur_page = (offset & 0xF00) >> 8;
	}

	return fragment_append(state->sect->current, byte);
}

static void codegen_register_special_idents(codegen_state *state, symtbl *tbl) {
	const insn_def *definition = &instruction_defs[0];
	while(definition->name) {
		state->idents.opcodes[definition->e_op] = symtbl_ident(tbl, string_from(definition->name, strlen(definition->name)));
		definition++;
	}

	const jcn_alias_def *alias_def = &jcn_alias_defs[0];
	while(alias_def->name) {
		state->idents.jcn_aliases[alias_def->e_alias] = symtbl_ident(tbl, string_from(alias_def->name, strlen(alias_def->name)));
		alias_def++;
	}

	const insn_pseudo_def *pseudo_def = &pseudo_insn_defs[0];
	while(pseudo_def->name) {
		state->idents.pseudos[pseudo_def->e_pseudo] = symtbl_ident(tbl, string_from(pseudo_def->name, strlen(pseudo_def->name)));
		pseudo_def++;
	}

	const index_reg_def *reg_def = &index_reg_defs[0];
	while(reg_def->name) {
		state->idents.registers[reg_def->e_reg] = symtbl_ident(tbl, string_from(reg_def->name, strlen(reg_def->name)));
		reg_def++;
	}
}

static uint8_t codegen_test_reg_pair(codegen_state *state, size_t reg1, size_t reg2) {
	if(reg1 == state->idents.registers[R0]  && reg2 == state->idents.registers[R1])  return 0b000;
	if(reg1 == state->idents.registers[R2]  && reg2 == state->idents.registers[R3])  return 0b001;
	if(reg1 == state->idents.registers[R4]  && reg2 == state->idents.registers[R5])  return 0b010;
	if(reg1 == state->idents.registers[R6]  && reg2 == state->idents.registers[R7])  return 0b011;
	if(reg1 == state->idents.registers[R8]  && reg2 == state->idents.registers[R9])  return 0b100;
	if(reg1 == state->idents.registers[R10] && reg2 == state->idents.registers[R11]) return 0b101;
	if(reg1 == state->idents.registers[R12] && reg2 == state->idents.registers[R13]) return 0b110;
	if(reg1 == state->idents.registers[R14] && reg2 == state->idents.registers[R15]) return 0b111;

	return 0xFF;
}

static uint8_t codegen_test_reg(codegen_state *state, size_t reg) {
	for(uint8_t i = 0; i < R_ESIZE; i++) {
		if(state->idents.registers[i] == reg) return i;
	}

	return 0xFF;
}

static uint16_t codegen_resolve_arg_number(codegen_state *state, arg *arg, enum insn_arg_type wanted) {
	const char *max_err = "**unexpected**";

	switch(wanted) {
		case at_half:
			if(arg->num > 0xF) {
				max_err = "4 bits";
				break;
			}
			return (uint16_t) arg->num;
		case at_byte:
			if(arg->num > 0xFF) {
				max_err = "8 bits";
				break;
			}
			return (uint16_t) arg->num;
		case at_triple:
			if(arg->num > 0xFFF) {
				max_err = "12 bits";
				break;
			}
			return (uint16_t) arg->num;
		case at_reg:
			position_error(arg->pos, "expected an index register name, but given a number\n");
			state->iserr = true;
			return RESOLVE_ERROR;
		case at_reg_pair:
			position_error(arg->pos, "expected a pair of index registers, but given a number\n");
			state->iserr = true;
			return RESOLVE_ERROR;
	}

	position_error(arg->pos, "number is too big, expected %s max\n", max_err);
	state->iserr = true;
	return RESOLVE_ERROR;
}

static uint16_t codegen_resolve_arg_ident(codegen_state *state, arg *arg, enum insn_arg_type wanted) {
	if(wanted == at_reg) {
		uint8_t code = codegen_test_reg(state, arg->ident);
		if(code == 0xFF) {
			position_error(arg->pos, "unsupported register: `%s`\n", symtbl_get(state->tbl, arg->ident));
			state->iserr = true;
		}
		return code;
	}

	if(wanted == at_reg_pair) {
		position_error(arg->pos, "expected a pair, but given an identifier `%s`\n", symtbl_get(state->tbl, arg->ident));
		state->iserr = true;
		return RESOLVE_ERROR;
	}

	return RESOLVE_REFERENCE;
}

static uint16_t codegen_resolve_arg_pair(codegen_state *state, arg *arg, enum insn_arg_type wanted) {
	if(wanted == at_reg_pair) {
		uint8_t code = codegen_test_reg_pair(state, arg->pair.ident1, arg->pair.ident2);
		if(code == 0xFF) {
			position_error(arg->pos, "unsupported register pair: `%s:%s`\n",
			               symtbl_get(state->tbl, arg->pair.ident1),
			               symtbl_get(state->tbl, arg->pair.ident2)
			);
			state->iserr = true;
		}
		return code;
	}

	const char *expected = "**unexpected**";
	switch(wanted) {
		case at_reg: expected = "an index register"; break;
		case at_half: expected = "4 bits number"; break;
		case at_byte: expected = "8 bits number"; break;
		case at_triple: expected = "12 bits number"; break;
		default: break;
	}
	position_error(arg->pos, "expected %s, but given a register pair\n", expected);
	state->iserr = true;
	return RESOLVE_ERROR;
}

static uint16_t codegen_resolve_arg(codegen_state *state, arg *arg, enum insn_arg_type wanted) {
	switch(arg->type) {
		case arg_ident: return codegen_resolve_arg_ident(state, arg, wanted);
		case arg_number: return codegen_resolve_arg_number(state, arg, wanted);
		case arg_pair: return codegen_resolve_arg_pair(state, arg, wanted);
	}
	position_error(arg->pos, "could not resolve argument\n");
	state->iserr = true;
	return RESOLVE_ERROR;
}

static bool codegen_verify_args_count(codegen_state *state, insn *insn, size_t args_count, size_t wanted) {
	if(args_count > wanted) {
		state->iserr = true;
		position_error(insn->pos, "instruction `%s` has too many arguments (%zu)\n",
		               symtbl_get(state->tbl, insn->op), args_count
		);
		return false;
	}
	if(args_count < wanted) {
		state->iserr = true;
		position_error(insn->pos, "instruction `%s` has too few arguments (%zu)\n",
		               symtbl_get(state->tbl, insn->op), args_count
		);
		return false;
	}
	return true;
}

static const insn_def *codegen_find_instruction(codegen_state *state, size_t ident) {
	for(int i = 0; i < OP_ESIZE; i++) {
		if(state->idents.opcodes[i] == ident) return &instruction_defs[i];
	}
	return 0;
}

static const jcn_alias_def *codegen_find_jcn_alias(codegen_state *state, size_t ident) {
	for(int i = 0; i < JCN_ESIZE; i++) {
		if(state->idents.jcn_aliases[i] == ident) return &jcn_alias_defs[i];
	}
	return 0;
}

static const insn_pseudo_def *codegen_find_pseudo(codegen_state *state, size_t ident) {
	for(int i = 0; i < PSEUDO_ESIZE; i++) {
		if(state->idents.pseudos[i] == ident) return &pseudo_insn_defs[i];
	}
	return 0;
}

static uint8_t codegen_transform_jcn(codegen_state *state, insn *insn) {
	for(int i = 0; i < JCN_ESIZE; i++) {
		if(state->idents.jcn_aliases[i] == insn->op) {
			return jcn_alias_defs[i].cond;
		}
	}
	return 0xFF;
}

static void codegen_create_reference(codegen_state *state, size_t ident, size_t offset, enum e_reference_type type, position pos) {
	reference *ref = malloc(sizeof(reference));
	ref->list.next = (list_head *) state->references;
	ref->ident = ident;
	ref->offset = offset;
	ref->type = type;
	ref->pos = pos;

	state->references = ref;
}

static void codegen_define_symbol(codegen_state *state, size_t ident, size_t offset, enum e_symbol_type type, position pos) {
	if(symbol_find(state->symbols, ident)) {
		fprintf(stderr, "error: double definition of symbol `%s`\n",
		        symtbl_get(state->tbl, ident)
		);
		state->iserr = true;
		return;
	}

	symbol *sym = malloc(sizeof(symbol));
	sym->list.next = (list_head *) state->symbols;
	sym->ident = ident;
	sym->offset = offset;
	sym->type = type;
	sym->pos = pos;

	state->symbols = sym;
}

static size_t codegen_write_jcn(codegen_state *state, uint8_t cond, arg *arg) {
	uint16_t addr = codegen_resolve_arg(state, arg, at_byte);

	size_t label_offset = codegen_write(state, instruction_defs[OP_JCN].opcode | cond, arg->pos);
	size_t ref_offset = codegen_write(state, (uint8_t) addr,  arg->pos);

	if(addr == RESOLVE_REFERENCE)
		codegen_create_reference(state, arg->ident, ref_offset, REF_BYTE, arg->pos);

	return label_offset;
}

static void codegen_handle_real_instruction(codegen_state *state, const insn_def *def, insn *insn) {
	size_t args_count = list_head_size(insn->args);

	size_t label_offset = 0;

	switch(def->e_args) {
		case ARG_TYPE_NONE:
			if(codegen_verify_args_count(state, insn, args_count, 0)) {
				label_offset = codegen_write(state, def->opcode, insn->pos);
			}
			break;
		case ARG_TYPE_1COND_2ADDR:
			if(codegen_verify_args_count(state, insn, args_count, 2)) {
				arg *first = insn->args;
				arg *second = (arg *) first->list.next;

				uint8_t cond = (uint8_t) codegen_resolve_arg(state, first, at_half);

				label_offset = codegen_write_jcn(state, cond, second);

				if((label_offset & 0xFF) >= 0xFE) {
					position_warning(insn->pos, "jcn instruction at last two bytes of page\n");
				}
			}
			break;
		case ARG_TYPE_1PAIR_2DATA:
			if(codegen_verify_args_count(state, insn, args_count, 2)) {
				arg *first = insn->args;
				arg *second = (arg *) first->list.next;

				uint8_t pair = (uint8_t) codegen_resolve_arg(state, first, at_reg_pair);
				uint16_t data = codegen_resolve_arg(state, second, at_byte);

				label_offset = codegen_write(state, def->opcode | (pair << 1), insn->pos);
				size_t ref_offset = codegen_write(state, (uint8_t) data, insn->pos);

				if(data == RESOLVE_REFERENCE)
					codegen_create_reference(state, second->ident, ref_offset, REF_BYTE, second->pos);
			}
			break;
		case ARG_TYPE_1PAIR:
			if(codegen_verify_args_count(state, insn, args_count, 1)) {
				arg *first = insn->args;

				uint8_t pair = (uint8_t) codegen_resolve_arg(state, first, at_reg_pair);

				label_offset = codegen_write(state, def->opcode | (pair << 1), insn->pos);
			}
			break;
		case ARG_TYPE_3ADDR:
			if(codegen_verify_args_count(state, insn, args_count, 1)) {
				arg *first = insn->args;

				uint16_t addr = codegen_resolve_arg(state, first, at_triple);

				label_offset = codegen_write(state, (uint8_t) (def->opcode | ((addr >> 8) & 0x0F)), insn->pos);
				codegen_write(state, (uint8_t) (addr & 0xFF), insn->pos);

				if(addr == RESOLVE_REFERENCE)
					codegen_create_reference(state, first->ident, label_offset, REF_LAST_HALF_2HALF, first->pos);
			}
			break;
		case ARG_TYPE_1REG:
			if(codegen_verify_args_count(state, insn, args_count, 1)) {
				arg *first = insn->args;

				uint8_t reg = (uint8_t) codegen_resolve_arg(state, first, at_reg);

				label_offset = codegen_write(state, def->opcode | reg, insn->pos);
			}
			break;
		case ARG_TYPE_1REG_2ADDR:
			if(codegen_verify_args_count(state, insn, args_count, 2)) {
				arg *first = insn->args;
				arg *second = (arg *) first->list.next;

				uint8_t reg = (uint8_t) codegen_resolve_arg(state, first, at_reg);
				uint16_t addr = codegen_resolve_arg(state, second, at_byte);

				label_offset = codegen_write(state, def->opcode | reg, insn->pos);
				size_t ref_offset = codegen_write(state, (uint8_t) addr, insn->pos);

				if((label_offset & 0xFF) >= 0xFE) {
					position_warning(insn->pos, "isz instruction at last two bytes of page\n");
				}

				if(addr == RESOLVE_REFERENCE)
					codegen_create_reference(state, second->ident, ref_offset, REF_BYTE, second->pos);
			}
			break;
		case ARG_TYPE_1DATA:
			if(codegen_verify_args_count(state, insn, args_count, 1)) {
				arg *first = insn->args;

				uint8_t data = (uint8_t) codegen_resolve_arg(state, first, at_half);

				label_offset = codegen_write(state, def->opcode | data, insn->pos);
			}
			break;
	}

	label *lbl = insn->lbls;
	while(lbl) {
		codegen_define_symbol(state, lbl->ident, label_offset, SYM_COMMON, lbl->pos);

		lbl = (label*) lbl->list.next;
	}
}

static void codegen_handle_jcn_alias(codegen_state *state, const jcn_alias_def *def, insn *insn) {
	size_t label_offset = 0;
	if(codegen_verify_args_count(state, insn, list_head_size(insn->args), 1)) {
		label_offset = codegen_write_jcn(state, def->cond, insn->args);
		if((label_offset & 0xFF) >= 0xFE) {
			position_warning(insn->pos, "jcn instruction at last two bytes of page\n");
		}
	}
	label *lbl = insn->lbls;
	while(lbl) {
		codegen_define_symbol(state, lbl->ident, label_offset, SYM_COMMON, lbl->pos);

		lbl = (label*) lbl->list.next;
	}
}

static void codegen_handle_pseudo(codegen_state *state, const insn_pseudo_def *def, insn *insn) {
	size_t label_offset = 0;
	if(def->e_pseudo == PSEUDO_DB) {
		arg *cur_arg = insn->args;
		bool label_not_assigned = true;
		while(cur_arg) {
			uint16_t val = codegen_resolve_arg(state, cur_arg, at_byte);

			size_t ref_offset = codegen_write(state, (uint8_t) val, insn->pos);
			if(label_not_assigned) {
				label_not_assigned = false;
				label_offset = ref_offset;
			}

			if(val == RESOLVE_REFERENCE)
				codegen_create_reference(state, cur_arg->ident, ref_offset, REF_BYTE, cur_arg->pos);

			cur_arg = (arg*) cur_arg->list.next;
		}
	}
	label *lbl = insn->lbls;
	while(lbl) {
		codegen_define_symbol(state, lbl->ident, label_offset, SYM_COMMON, lbl->pos);

		lbl = (label*) lbl->list.next;
	}
}

static void codegen_org(codegen_state *state, size_t offset) {
	if(offset == 0x00 && state->sect->frag->len == 0) {
		state->sect->current = state->sect->frag;
	} else {
		section_create_current(state->sect, offset);
	}
}

static void codegen_handle(codegen_state *state, insn *insn) {
	directive *dir = insn->dirs;
	while(dir) {
		switch(dir->dir) {
			case DIR_PAGE:
				state->cur_page = dir->num;
				codegen_org(state, dir->num << 8);
				break;
			case DIR_ORG:
				codegen_org(state, dir->num);
				break;
			default: break;
		}
		dir = (directive *) dir->list.next;
	}

	const insn_def *def = codegen_find_instruction(state, insn->op);
	if(def) {
		codegen_handle_real_instruction(state, def, insn);
		return;
	}

	const jcn_alias_def *alias_def = codegen_find_jcn_alias(state, insn->op);
	if(alias_def) {
		codegen_handle_jcn_alias(state, alias_def, insn);
		return;
	}

	const insn_pseudo_def *pseudo_def = codegen_find_pseudo(state, insn->op);
	if(pseudo_def) {
		codegen_handle_pseudo(state, pseudo_def, insn);
		return;
	}

	position_error(insn->pos, "unsupported instruction: `%s`\n", symtbl_get(state->tbl, insn->op));
	state->iserr = true;
}

codegen_state *codegen_from_insnlist(insn *insnlist, symtbl *tbl) {
	codegen_state *state = malloc(sizeof(codegen_state));
	state->sect = section_create();
	state->iserr = false;
	state->tbl = tbl;

	state->symbols = 0;
	state->references = 0;
	state->cur_page = 0;

	codegen_register_special_idents(state, tbl);

	insn *cur_insn = insnlist;
	while(cur_insn) {
		codegen_handle(state, cur_insn);

		cur_insn = (insn *) cur_insn->list.next;
	}

	return state;
}

void codegen_destroy(codegen_state *state) {
	list_head_destroy(state->references);
	list_head_destroy(state->symbols);

	section_destroy(state->sect);

	free(state);
}