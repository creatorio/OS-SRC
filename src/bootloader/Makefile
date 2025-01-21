BUILD_DIR?=build/
ASM?=nasm
SOURCES_C=$(wildcard *.c)
OBJECTS_C=$(patsubst %.c, $(BUILD_DIR)/%.o, $(SOURCES_C))

.PHONY: all clean

all: stage1

stage1: $(BUILD_DIR)/stage1.bin

#$(OBJECTS_C): $(SOURCES_C)
# compile: (flags before -o become CFLAGS in the Makefile)
#	 -c -o $(BUILD_DIR)/$<.o $<
$(BUILD_DIR)/stage1.bin: $(OBJECTS_C)
# link: (flags before -o become LDFLAGS in the Makefile)
	@x86_64-w64-mingw32-gcc -ffreestanding -I/home/osmaker/gnu-efi-3.0.18/inc -I/home/osmaker/gnu-efi-3.0.18/inc/x86_64 -I/home/osmaker/gnu-efi-3.0.18/inc/protocol -c -o $(BUILD_DIR)/data.o /home/osmaker/gnu-efi-3.0.18/lib/data.c
	@x86_64-w64-mingw32-gcc -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main -o $(BUILD_DIR)/BOOTX64.EFI $(OBJECTS_C) $(BUILD_DIR)/data.o
	@echo "--> Created BOOTX64.EFI"
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	@x86_64-w64-mingw32-gcc -ffreestanding -I/home/osmaker/gnu-efi-3.0.18/inc -I/home/osmaker/gnu-efi-3.0.18/inc/x86_64 -I/home/osmaker/gnu-efi-3.0.18/inc/protocol -c -o $@ $<
	@echo "--> Compiled: " $<


clean:
	@rm -f $(BUILD_DIR)/BOOTX64.EFI