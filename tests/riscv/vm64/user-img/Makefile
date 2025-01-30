img:
	mkdir -p build
	riscv64-linux-gnu-gcc -march=rv64i -mabi=lp64 -static -nostdlib -nostartfiles ./user.S -o ./build/user.elf
	riscv64-linux-gnu-objcopy -O binary -j .text ./build/user.elf ./build/user.bin
