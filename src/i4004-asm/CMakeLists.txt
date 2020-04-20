add_executable(i4004-asm main.c
        lexer.c parser.c
        string.c symtbl.c
        codegen.c section.c
        list_head.c linker.c
        error.c preproc.c
)