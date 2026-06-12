# Makefile for BSIT338OS

CC = i686-elf-gcc
AS = nasm
LD = i686-elf-gcc
GRUB_MKRESCUE = i686-elf-grub-mkrescue

CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Ikernel
LDFLAGS = -ffreestanding -O2 -nostdlib -T linker.ld -lgcc

# Source files
AS_SOURCES = boot.asm $(wildcard kernel/*.asm)
C_SOURCES = $(wildcard kernel/*.c)

# Object files
AS_OBJS = $(patsubst %.asm,%.o,$(AS_SOURCES))
C_OBJS = $(C_SOURCES:.c=.o)
OBJS = $(AS_OBJS) $(C_OBJS)

.PHONY: all clean run

all: myos.iso

%.o: %.asm
	$(AS) -f elf32 $< -o $@

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

kernel.bin: $(OBJS) linker.ld
	$(LD) -o $@ $(OBJS) $(LDFLAGS)

myos.iso: kernel.bin
	mkdir -p isodir/boot/grub
	cp kernel.bin isodir/boot/kernel.bin
	echo 'set timeout=0' > isodir/boot/grub/grub.cfg
	echo 'set default=0' >> isodir/boot/grub/grub.cfg
	echo '' >> isodir/boot/grub/grub.cfg
	echo 'menuentry "myos" {' >> isodir/boot/grub/grub.cfg
	echo '  multiboot /boot/kernel.bin' >> isodir/boot/grub/grub.cfg
	echo '  boot' >> isodir/boot/grub/grub.cfg
	echo '}' >> isodir/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o myos.iso isodir

run: myos.iso
	qemu-system-i386 -cdrom myos.iso -serial stdio

clean:
	rm -rf $(OBJS) kernel.bin myos.iso isodir
