%include "vm.inc"

    fim r0:r1, 0xF5
    src r0:r1
    ldm 0xF
    wrm

    vm_sleep
    vm_terminate