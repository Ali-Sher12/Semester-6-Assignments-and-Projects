default: main.s
	rm -rf main
	riscv64-linux-gnu-as main.s -o main.o
	riscv64-linux-gnu-ld -o main main.o 
	
run:
	clear
	qemu-riscv64-static ./main	

all:
	rm -rf main
	riscv64-linux-gnu-as main.s -o main.o
	riscv64-linux-gnu-ld -o main main.o 
	clear
	qemu-riscv64-static ./main		
