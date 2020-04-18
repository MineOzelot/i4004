        fim r0:r1, 0xFF
        jms and
        ld  r0
        wrr

        fim r0:r1, 0xCF
        jms and
        ld  r0
        wrr

        fim r0:r1, 0xBA
        jms and
        ld  r0
        wrr

        fim r0:r1, 0xEB
        jms and
        ld r0
        wrr

        fim r8:r9, 0xF
        src r8:r9
        ldm 1
        wr0

and:    fim r2:r3, 11
l1:     ldm 0
        xch r0
        ral
        xch r0
        inc r3
        xch r3
        jcn 4, l2
        xch r3
        rar
        xch r2
        xch r1
        ral
        xch r1
        rar
        add r2
        jun l1
l2:     bbl 0