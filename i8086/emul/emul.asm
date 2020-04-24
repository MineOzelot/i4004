%include "vm.inc"
%include "ram.inc"

print:
    fim r4:r5, 0

    fim_src r8:r9, 0xC6
    wr3
    rdm

    xch r4
    add r4

    ram_write r8:r9, 0xC0
    ram_read r8:r9, 0xC7

    xch r4
    add r4
    add r5
    xch r4

    ram_write r8:r9, 0xC1

    wr0

    xch r4
    jnz print

    vm_terminate
