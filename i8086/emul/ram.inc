%macro  fim_src index, addr
        fim index, addr
        src index
%endmacro

%macro  ram_read index, addr
        fim_src index, addr
        rdm
%endmacro

%macro  ram_write index, addr
        fim_src index, addr
        wrm
%endmacro
