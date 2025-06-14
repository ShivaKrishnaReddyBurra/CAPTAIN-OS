CC = x86_64-linux-gnu-gcc
LD = x86_64-linux-gnu-ld
AS = nasm
QEMU = qemu-system-x86_64
CFLAGS = -ffreestanding -mno-red-zone -m64 -c
LDFLAGS = -T linker.ld -nostdlib
ASF = -f elf64

all: captainos.iso

boot.o: boot/boot.asm
	$(AS) $(ASF) boot/boot.asm -o build/boot.o

kernel.o: kernel/kernel.c
	$(CC) $(CFLAGS) kernel/kernel.c -o build/kernel.o

captainos.bin: boot.o kernel.o
	$(LD) $(LDFLAGS) -o build/captainos.bin build/boot.o build/kernel.o

captainos.iso: captainos.bin
	mkdir -p iso/boot/grub
	cp build/captainos.bin iso/boot/captainos.bin
	cp grub/grub.cfg iso/boot/grub/
	grub-mkrescue -o build/captainos.iso iso

run: captainos.iso
	$(QEMU) -cdrom build/captainos.iso -d int -no-reboot -no-shutdown

clean:
	rm -rf build/* iso/

.PHONY: all run clean