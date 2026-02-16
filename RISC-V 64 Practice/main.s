.global _start

_start:
	li a7, 64
	li a0, 1
	la a1, hello
	li a2, 10
	ecall

	li a7, 93
	li a0, 0
	ecall

hello:
	.ascii "Hello Bro\n"
