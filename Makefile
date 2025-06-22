CC = x86_64-linux-gnu-gcc
LD = x86_64-linux-gnu-ld
AS = nasm 
QEMU = qemu-system-x86_64
CFLAGS = -ffreestanding -mno-red-zone -m64 -c -Iinclude
LDFLAGS = -T linker.ld -nostdlib
ASF = -f elf64

all: captainos.iso

boot.o: boot/boot.asm
	$(AS) $(ASF) boot/boot.asm -o build/boot.o

kernel.o: kernel/kernel.c
	$(CC) $(CFLAGS) kernel/kernel.c -o build/kernel.o

idt.o: kernel/idt.c
	$(CC) $(CFLAGS) kernel/idt.c -o build/idt.o

pic.o: kernel/pic.c
	$(CC) $(CFLAGS) kernel/pic.c -o build/pic.o
	
vga.o: kernel/vga.c
	$(CC) $(CFLAGS) kernel/vga.c -o build/vga.o

utils.o: kernel/utils.c
	$(CC) $(CFLAGS) kernel/utils.c -o build/utils.o

pit.o: kernel/pit.c
	$(CC) $(CFLAGS) kernel/pit.c -o build/pit.o

task.o: kernel/task.c
	$(CC) $(CFLAGS) kernel/task.c -o build/task.o

isr.o: kernel/isr.asm
	$(AS) $(ASF) kernel/isr.asm -o build/isr.o

framebuffer.o: kernel/framebuffer.c
	$(CC) $(CFLAGS) kernel/framebuffer.c -o build/framebuffer.o

captainos.bin: boot.o kernel.o idt.o pic.o vga.o utils.o pit.o task.o isr.o framebuffer.o 
	$(LD) $(LDFLAGS) -o build/captainos.bin build/boot.o build/kernel.o build/idt.o build/pic.o build/vga.o build/utils.o build/pit.o build/task.o build/isr.o build/framebuffer.o

captainos.iso: captainos.bin
	mkdir -p iso/boot/grub
	cp build/captainos.bin iso/boot/
	cp grub/grub.cfg iso/boot/grub/
	grub-mkrescue -o build/captainos.iso iso

# Best graphics mode for most systems
run: captainos.iso
	$(QEMU) -cdrom build/captainos.iso -boot d -no-reboot -no-shutdown -monitor stdio -k en-us -vga std -m 512M -enable-kvm

# Alternative graphics modes to try
run-cirrus: captainos.iso
	$(QEMU) -cdrom build/captainos.iso -boot d -no-reboot -no-shutdown -monitor stdio -k en-us -vga cirrus -m 512M

run-vmware: captainos.iso
	$(QEMU) -cdrom build/captainos.iso -boot d -no-reboot -no-shutdown -monitor stdio -k en-us -vga vmware -m 512M

run-qxl: captainos.iso
	$(QEMU) -cdrom build/captainos.iso -boot d -no-reboot -no-shutdown -monitor stdio -k en-us -vga qxl -m 512M

# Force VBE/VESA graphics mode
run-vbe: captainos.iso
	$(QEMU) -cdrom build/captainos.iso -boot d -no-reboot -no-shutdown -monitor stdio -k en-us -vga std -m 512M -device VGA,vgamem_mb=16

# Run with better graphics support (recommended for your system)
run-graphics: captainos.iso
	$(QEMU) -cdrom build/captainos.iso -boot d -no-reboot -no-shutdown -monitor stdio -k en-us -vga cirrus -m 512M -display gtk

# Run without KVM (if KVM causes issues)
run-no-kvm: captainos.iso
	$(QEMU) -cdrom build/captainos.iso -boot d -no-reboot -no-shutdown -monitor stdio -k en-us -vga std -m 512M

# Run without graphics (text mode only)
run-text: captainos.iso
	$(QEMU) -cdrom build/captainos.iso -boot d -no-reboot -no-shutdown -monitor stdio -k en-us -vga none -m 512M

# Debug run with more verbose output
run-debug: captainos.iso
	$(QEMU) -cdrom build/captainos.iso -boot d -d int,guest_errors -no-reboot -no-shutdown -monitor stdio -k en-us -vga std -m 512M -serial stdio

clean:
	rm -rf build/* iso/

.PHONY: all run run-cirrus run-vmware run-qxl run-vbe run-graphics run-no-kvm run-text run-debug clean