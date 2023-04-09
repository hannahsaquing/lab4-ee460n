; user program

    .ORIG x3000
 ; initialize memory location x4000 to 1
    
    LEA R0, FIRST
    LDW R2, R0, #0
    AND R1, R1, #0
    ADD R1, R1, #1
    STW R1, R2, #0

 ; calculate sum of first 20 bytes

    AND R0, R0, #0
    LEA R0, SECOND
    LDW R2, R0, #0 ; R2 has xC000 now
    
    AND R3, R3, #0
    AND R5, R5, #0
    ADD R3, R3, #20

sum LDB R4, R2, #0
    ADD R5, R5, R4 ; add sum to R5
    ADD R2, R2, #1 ; increment where we are adding
    ADD R3, R3, #-1; decrement how many more times we need to add a byte
    BRp sum

 ; store sum in xC014
    AND R0, R0, #0
    AND R1, R1, #0
    LEA R0, THIRD
    LDW R1, R0, #0
    STW R5, R1, #0

 ; cause protection exception by storing at x0000
    AND R0, R0, #0
    STW R5, R0, #0
 
 ; cause unaligned access exception by storing at xC017
    AND R1, R1, #0
    LEA R0, THIRD
    LDW R1, R0, #0
    ADD R1, R1, #3
    STW R5, R1, #0

 ; cause unknown opcode by using .FILL psuedo op with 1010 and 1011
    .FILL xA000
    .FILL xB000


first   .FILL x4000
second  .FILL xC000
third   .FILL xC014

    .END