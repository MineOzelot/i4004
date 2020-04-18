#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "linker.h"

static int start_assembler(const char *in_file, const char *out_file) {
	FILE *input = fopen(in_file, "r");
	if(!input) {
		fprintf(stderr, "error: could not open input file\n");
		return 1;
	}

	int ret_val = 1;

	lexer_state *lexer = 0;
	parser_state *parser = 0;
	codegen_state *codegen = 0;
	linker_state *linker = 0;

	FILE *output = 0;

	lexer = lexer_start(input, in_file);
	parser = parser_start(lexer);

	parser_parse(parser);
	if(lexer->iserr || parser->iserr) goto destroy;

	insn *cur_insn = parser_get(parser);
	codegen = codegen_from_insnlist(cur_insn, lexer->symtbl);
	if(codegen->iserr) goto destroy;

	linker = linker_create(lexer->symtbl);

	linker_put_section(linker, codegen->sect);
	if(linker->iserr) goto destroy;

	linker_link(linker, codegen->symbols, codegen->references);
	if(linker->iserr) goto destroy;

	output = fopen(out_file, "wb");
	if(!output) {
		fprintf(stderr, "error: could not create output file\n");
		goto destroy;
	}

	fwrite(linker->sect, SECTION_SIZE_MAX, 1, output);

	ret_val = 0;
destroy:
	if(output) fclose(output);
	if(linker) linker_destroy(linker);
	if(codegen) codegen_destroy(codegen);
	if(parser) parser_end(parser);
	return ret_val;
}

int main(int argc, char *argv[]) {
	int opt;
	opterr = 0;

	const char *in_file = 0;
	const char *out_file = "out.i4004";

	while((opt = getopt(argc, argv, "o:")) != -1) {
		switch(opt) {
			case 'o':
				out_file = optarg;
				break;
			case '?':
				if(optopt == 'o') {
					fprintf(stderr, "error: missing filename after -%c\n", optopt);
				} else if(isprint(optopt)) {
					fprintf(stderr, "error: unrecognized option `-%c`\n", optopt);
				} else {
					fprintf(stderr, "error: unknown option character `\\x%x`\n", optopt);
				}
				return 1;
			default:
				abort();
		}
	}

	if(optind >= argc) {
		fprintf(stderr, "error: no input file\n");
		return 1;
	}

	in_file = argv[optind];

	if(optind + 1 < argc) {
		fprintf(stderr, "error: too many input files\n");
		return 1;
	}

	return start_assembler(in_file, out_file);
}