%include "vm.inc"

    ldm 0x1
    fim r0:r1, 0xA

    sub r1
    jz  equal

    fim r8:r9, 0xC0
    src r8:r9
    ldm 0x6
    wrm
    fim r8:r9, 0xC1
    src r8:r9
    ldm 0x4
    wrm
    wr0

    vm_terminate

equal:

    fim r8:r9, 0xC0
    src r8:r9
    ldm 0x4
    wrm
    fim r8:r9, 0xC1
    src r8:r9
    ldm 0x5
    wrm
    wr0

    vm_terminate