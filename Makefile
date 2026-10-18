# Cross-Compiler Toolchain
CXX = i686-elf-g++
AS = i686-elf-as
LD = i686-elf-ld
QEMU = qemu-system-i386

# Bare-metal flags
CXXFLAGS = -ffreestanding -fno-exceptions -fno-rtti -std=c++17 -Wall -Wextra
LDFLAGS = -m elf_i386 -T linker.ld

BIN = ccos.bin
DISK = disk.img

all: $(BIN)

boot.o: boot.s
	$(AS) boot.s -o boot.o

kernel.o: kernel.cc kernel.h disk.h
	$(CXX) $(CXXFLAGS) -c kernel.cc -o kernel.o

$(BIN): boot.o kernel.o linker.ld
	$(LD) $(LDFLAGS) boot.o kernel.o -o $(BIN)

$(DISK):
	@if [ ! -f $(DISK) ]; then \
		dd if=/dev/zero of=$(DISK) bs=1M count=512; \
	fi

run: all $(DISK)
	$(QEMU) -kernel $(BIN) -drive file=$(DISK),format=raw,if=ide,index=0,media=disk

clean:
	rm -f *.o $(BIN)

clean-all: clean
	rm -f $(DISK)

.PHONY: all run clean clean-all