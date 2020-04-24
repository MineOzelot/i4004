%macro  fim_src reg, addr
        fim reg, addr
        src reg
%endmacro

%macro  vm_set_state new_state
        fim_src r0:r1, 0xF0
        ldm new_state
        wr0
%endmacro

%macro  vm_terminate
        vm_set_state 0x1
%endmacro

%macro  vm_sleep
        vm_set_state 0x2
%endmacro
