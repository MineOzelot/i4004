%macro  vm_set_state new_state
        fim r0:r1, 0xF0
        src r0:r1
        ldm new_state
        wr0
%endmacro

%macro  vm_terminate
        vm_set_state 0x1
%endmacro

%macro  vm_sleep
        vm_set_state 0x2
%endmacro
