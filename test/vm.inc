%macro  vm_terminate
        fim r0:r1, 0xF
        src r0:r1
        ldm 1
        wr0
%endmacro
