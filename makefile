# Makefile
all: diskimage bootloader kernel

# Recipes
diskimage:
	dd if=/dev/zero of=out/system.img bs=512 count=2880

bootloader:
	nasm src/asm/bootloader.asm -o out/bootloader
	dd if=out/bootloader of=out/system.img bs=512 count=1 conv=notrunc

kernel:
	bcc -ansi -c -o out/kernel.o src/c/kernel.c
	bcc -ansi -c -o out/std_lib.o src/c/std_lib.c
	bcc -ansi -c -o out/shell.o src/c/shell.c
	nasm -f as86 src/asm/kernel.asm -o out/kernel_asm.o
	ld86 -o out/kernel -d out/*.o
	dd if=out/kernel of=out/system.img bs=512 conv=notrunc seek=1

run:
	bochs -f src/config/if2230.config

build-run: all run

tc:
	cd out && ./tc_gen A

tc-2:
	cd out && ./tc_gen B

tc-3:
	cd out && ./tc_gen C

tc-4:
	cd out && ./tc_gen D

tcA: tc run

tcB: tc-2 run

tcC: tc-3 run

tcD: tc-4 run
