.data
a:  db 0x12, 0x34, 0x56; comment
.code
entry:    ;another comment
    nop
jcns:
    jcn 0b0101, fims
    jnt srcs
    jc 0xAB
    jz 0xAB
    jt 0xAB
    jnc 0xAB
    jnz 0xAB
fims:
srcs:
    fim r0:r1, one_operand
    src r0:r1
    src r2:r3
    src r4:r5
    src r6:r7
    src r8:r9
    src r10:r11
    src r12:r13
    src r14:r15

    fin r2:r3
    jin r2:r3

    jun jcns
    jms 0x456
    inc r5
    isz r6, 0xBE
    add r7
    sub r8
    ld r9
    xch r10
    bbl 0x2
    ldm 0x4
one_operand:
    wrm
    wmp
    wrr
    wpm
    wr0
    wr1
    wr2
    wr3
    sbm
    rdm
    rdr
    adm
    rd0
    rd1
    rd2
    rd3
    clb
    clc
    iac
    cmc
    cma
    ral
    rar
    tcc
    dac
    tcs
    stc
    daa
    kbp
    dcl
