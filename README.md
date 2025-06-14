# CAPTAIN-OS

CAPTAIN-OS is a minimal operating system kernel written in Assembly and C, designed as a learning project for OS development. It uses the Multiboot2 specification to boot via GRUB, transitions from 32-bit to 64-bit mode, and displays a "Hello, CAPTAIN_SKR" message on the screen using the VGA text buffer. This project demonstrates fundamental OS concepts like bootloader setup, kernel initialization, and low-level hardware interaction.

## Features

- Boots using GRUB with the Multiboot2 specification.
- Transitions from 32-bit to 64-bit mode during boot.
- Displays "Hello, World!" in VGA text mode (white text on black background).
- Minimal setup with a Global Descriptor Table (GDT) and basic paging for 64-bit mode.

## Project Structure

- **`boot/`**: Contains the bootloader code.
  - `boot.asm`: Assembly code that defines the Multiboot2 header, sets up the initial environment, and transitions from 32-bit to 64-bit mode.
- **`build/`**: Directory for build artifacts (e.g., object files, `captainos.bin`, `captainos.iso`).
- **`grub/`**: Contains GRUB configuration.
  - `grub.cfg`: GRUB configuration file to load the kernel using Multiboot2.
- **`iso/`**: Directory used to create the ISO image for booting.
- **`kernel/`**: Contains the kernel code.
  - `kernel.c`: C code for the kernel, which writes "Hello, World!" to the VGA text buffer.
- **`.gitignore`**: Git ignore file to exclude build artifacts and temporary files.
- **`linker.ld`**: Linker script to define the memory layout of the kernel.
- **`Makefile`**: Build script to compile, link, and create the bootable ISO.
- **`README.md`**: This file, providing an overview and instructions for the project.

## Prerequisites

To build and run CAPTAIN-OS, you’ll need the following tools installed on a Linux system (e.g., Ubuntu):

- **NASM**: Assembler for compiling `boot.asm`.
  - Install: `sudo apt install nasm`
- **GCC (x86_64 cross-compiler)**: Compiler for `kernel.c`.
  - Install: `sudo apt install gcc-x86-64-linux-gnu`
- **LD (x86_64 linker)**: Linker for creating the kernel binary.
  - Install: `sudo apt install binutils-x86-64-linux-gnu`
- **GRUB Tools**: For creating the bootable ISO.
  - Install: `sudo apt install grub-pc-bin grub-efi-amd64-bin xorriso`
- **QEMU**: Emulator to run the OS.
  - Install: `sudo apt install qemu-system-x86`

## Installation

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/ShivaKrishnaReddyBurra/CAPTAIN-OS.git
   cd CAPTAIN-OS
   ```

2. **Install Dependencies**:
   Ensure all prerequisites are installed (see above). On Ubuntu, you can run:
   ```bash
   sudo apt update
   sudo apt install nasm gcc-x86-64-linux-gnu binutils-x86-64-linux-gnu grub-pc-bin grub-efi-amd64-bin xorriso qemu-system-x86
   ```

## Usage

1. **Build the Kernel**:
   The `Makefile` automates the build process. Run:
   ```bash
   make
   ```
   This will:
   - Assemble `boot.asm` into `build/boot.o`.
   - Compile `kernel.c` into `build/kernel.o`.
   - Link the object files into `build/captainos.bin` (ELF format).
   - Create a bootable ISO (`build/captainos.iso`) using GRUB.

2. **Run the Kernel in QEMU**:
   ```bash
   make run
   ```
   This will launch QEMU and boot the kernel from the ISO. You should see "Hello, World!" displayed on the screen in white text on a black background.

3. **Clean Build Artifacts** (optional):
   ```bash
   make clean
   ```
   This removes the `build/` and `iso/` directories.

## How It Works

1. **Bootloader (`boot.asm`)**:
   - Defines a Multiboot2 header with architecture set to i386 (32-bit), compatible with the i386-pc GRUB environment.
   - Starts in 32-bit mode (as GRUB boots in 32-bit mode), sets up a stack, and transitions to 64-bit mode by:
     - Setting up a Global Descriptor Table (GDT) for 64-bit mode.
     - Enabling Physical Address Extension (PAE) and Long Mode.
     - Setting up basic paging with an identity mapping for the first 2MB.
     - Jumping to 64-bit code.
   - Calls the C kernel entry point (`kernel_main`).

2. **Kernel (`kernel.c`)**:
   - Writes "Hello, CAPTAIN_SKR" to the VGA text buffer at address `0xB8000`, displaying the message in white text on a black background.
   - Enters an infinite loop to prevent the kernel from exiting.

3. **GRUB Configuration (`grub.cfg`)**:
   - Loads the kernel (`captainos.bin`) as an ELF file using the `multiboot2` command.
   - Boots the kernel immediately (no timeout).




## Acknowledgments

- **Multiboot2 Specification**: For providing a standard way to boot the kernel via GRUB.
- **OSDev Wiki**: A valuable resource for OS development tutorials and documentation.
- **QEMU and GRUB**: Essential tools for testing and booting the kernel.

