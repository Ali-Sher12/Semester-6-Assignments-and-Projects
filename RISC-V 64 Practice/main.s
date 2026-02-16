.section .data
hello:
    .ascii "Hello Bmro\n"

.section .text
.global _start

_start:
    li a0, 5          # argument: n = 5
    call add_ten      # call function
    # result now in a0 (15)
    
    li a7, 64
    li a0, 1
    la a1, hello
    li a2, 10
    ecall
    
    li a7, 93
    li a0, 0
    ecall

# Simple function: adds 10 to the argument
# Input: a0 = number
# Output: a0 = number + 10
add_ten:
    addi a0, a0, 10   # a0 = a0 + 10
    ret               # return to caller