%macro  vm_terminate
        fim r0:r1, 0xF0
        src r0:r1
        ldm 1
        wr0
%endmacro

%macro  vm_sleep
        fim r0:r1, 0xF0
        src r0:r1
        ldm 2
        wr0
%endmacro