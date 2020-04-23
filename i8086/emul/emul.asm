%include "vm.inc"

print:
    fim r4:r5, 0

    fim r8:r9, 0xC6
    src r8:r9
    wr3
    rdm

    xch r4
    add r4

    fim r8:r9, 0xC0
    src r8:r9
    wrm

    fim r8:r9, 0xC7
    src r8:r9
    rdm

    xch r4
    add r4
    add r5
    xch r4

    fim r8:r9, 0xC1
    src r8:r9
    wrm

    wr0

    xch r4
    jnz print

    vm_terminate
