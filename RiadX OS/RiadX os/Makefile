# Makefile for RiadX-OS Kernel
# Adjust CROSS_PREFIX if your cross-compiler uses a different prefix
CROSS_PREFIX = i686-elf-
CC = $(CROSS_PREFIX)g++
LD = $(CROSS_PREFIX)ld
OBJCOPY = $(CROSS_PREFIX)objcopy
CFLAGS = -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti -nostdlib -m32
LDFLAGS = -T linker.ld -nostdlib -static -m elf_i386

SRC = $(wildcard *.cpp)
OBJ = $(SRC:.cpp=.o)
TARGET = kernel.bin

.PHONY: all clean run iso

all: $(TARGET)

$(TARGET): $(OBJ) linker.ld
	$(LD) $(LDFLAGS) -o kernel.elf $(OBJ)
	$(OBJCOPY) -O binary kernel.elf $@

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	qemu-system-i386 -kernel $(TARGET)

iso: all
	mkdir -p iso/boot/grub
	cp $(TARGET) iso/boot/kernel.bin
	echo 'set timeout=0' > iso/boot/grub/grub.cfg
	echo 'set default=0' >> iso/boot/grub/grub.cfg
	echo 'menuentry "RiadX-OS" { multiboot /boot/kernel.bin }' >> iso/boot/grub/grub.cfg
	grub-mkrescue -o RiadX-OS.iso iso

clean:
	rm -f *.o *.elf $(TARGET)
	rm -rf iso RiadX-OS.iso
