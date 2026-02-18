.section .data
hello:
    .ascii "Hello Bmro\n"

.section .text
.global _start

_start:
    li t0, 1
    li t1, 5
    call factorial

    li a7, 93
    addi a0,t0,0
    ecall

factorial:
    #stored return address in stack
    addi sp,sp,-8
    sd ra,0(sp)

    beq t1,zero,done
    mul t0,t0,t1
    addi t1,t1,-1    
    call factorial
done:

    ld ra,0(sp)
    addi sp,sp,8
    ret
