        .ORIG x3000
        LEA R0, FIRST ; r0 <-x300C
        LDW R1, R0, #0 ; r1 <- x4000
        LDW R2, R1, #0 ; r2 <- mem[x4000]
        ADD R2, R2, #1 ; r2 <- mem[x4000] + 1
        STW R2, R1, #0 ; mem[x4000] + 1 -> x4000
FIRST   .FILL x4000

 ; calculate sum of first 20 bytes

        AND R0, R0, #0
        AND R1, R1, #0
        AND R2, R2, #0
        AND R5, R5, #0
        
        LEA R0, SECOND ; get starting address
        LDW R1, R0, #0 
        LDW R2, R1, #0 
        AND R0, R0, #0
        AND R1, R1, #0
        ADD R0, R0, #10
        ADD R1, R1, #10
SECOND  .FILL xC000

        ; Get bytes and add them together

SUM     LDB R4, R2, #0
        ADD R5, R5, R4
        ADD R2, R2, #1 ; increment address
        ADD R0, R0, #-1
        BRp SUM
        ADD R1, R1, #-1
        BRp SUM

        ; store sum in xC014
        AND R0, R0, #0
        AND R1, R1, #0
        AND R2, R2, #0

        LEA R0, THIRD ; get starting address
        LDW R1, R0, #0 
        LDW R2, R1, #0 ; R2 <- xC014
        STW R5, R2, #0
THIRD   .FILL xC014

        ; cause protection exception
        ;AND R0, R0, #0
        ;STW R5, R0, #0

        ;cause unaligned access exception
        ;AND R1, R1, #0
        ;LEA R0, THIRD
        ;LDW R1, R0, #0
        ;ADD R1, R1, #3
        ;STW R5, R1, #0

        ; .fill xa000
        ; .fill xb000
        .END