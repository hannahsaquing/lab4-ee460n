    ; interrupt service routine

    .ORIG x1200

; push R0 and R1
    ADD R6, R6, #-2
    STW R0, R6, #0
    ADD R6, R6, #-2
    STW R1, R6, #0
    ADD R6, R6, #-2
    STW R2, R6, #0

; increment x4000
    LEA R0, A
    LDW R1, R0, #0 ; R1 = x4000
    LDW R2, R1, #0 ; R2 = MEM[x4000]
    ADD R2, R2, #1
    STW R2, R1, #0

; pop R0 and R1
    LDW R2, R6, #0
    ADD R6, R6, #2
    LDW R1, R6, #0
    ADD R6, R6, #2
    LDW R0, R6, #0
    ADD R6, R6, #2

    RTI
a   .FILL x4000

    .END