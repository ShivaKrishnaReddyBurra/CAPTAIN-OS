CC = gcc
LD = ld
AS = nasm 
QEMU = qemu-system-x86_64
CFLAGS = -m64 -ffreestanding -fno-stack-protector -fno-builtin -nostdlib -Wall -Wextra -c -mno-red-zone -O0
LDFLAGS = -T linker.ld -nostdlib
ASFLAGS = -f elf64

# Create build directory if it doesn't exist
$(shell mkdir -p build iso/boot/grub)

all: captainos.iso

build/boot.o: boot/boot.asm
	@echo "Assembling boot.asm..."
	$(AS) $(ASFLAGS) boot/boot.asm -o build/boot.o
	@echo "Boot object created successfully"

build/kernel.o: kernel/kernel.c
	@echo "Compiling kernel/kernel.c..."
	$(CC) $(CFLAGS) kernel/kernel.c -o build/kernel.o
	@echo "Kernel object created successfully"

build/captainos.bin: build/boot.o build/kernel.o linker.ld
	@echo "Linking kernel..."
	$(LD) $(LDFLAGS) -o build/captainos.bin build/boot.o build/kernel.o
	@echo "Kernel binary created successfully"
	@echo "Kernel size: $$(stat -c%s build/captainos.bin) bytes"

build/captainos.iso: build/captainos.bin grub/grub.cfg
	@echo "Creating ISO..."
	cp build/captainos.bin iso/boot/
	cp grub/grub.cfg iso/boot/grub/
	grub-mkrescue -o build/captainos.iso iso 2>/dev/null
	@echo "ISO created successfully"
	@echo "ISO size: $$(stat -c%s build/captainos.iso) bytes"

captainos.iso: build/captainos.iso

run: build/captainos.iso
	@echo "Starting QEMU..."
	$(QEMU) -cdrom build/captainos.iso -m 512M

debug: build/captainos.iso
	@echo "Starting QEMU with debug output..."
	$(QEMU) -cdrom build/captainos.iso -m 512M -d int,cpu_reset -no-reboot -no-shutdown -serial stdio

test: build/captainos.iso
	@echo "Testing kernel boot with minimal output..."
	timeout 10s $(QEMU) -cdrom build/captainos.iso -m 512M -display none -serial stdio || echo "Boot test completed"

objdump: build/captainos.bin
	@echo "Disassembling kernel..."
	objdump -d build/captainos.bin

info: build/captainos.bin
	@echo "Kernel information:"
	@file build/captainos.bin
	@size build/captainos.bin
	@readelf -h build/captainos.bin

clean:
	rm -rf build/* iso/boot/*

.PHONY: all run debug test objdump info clean