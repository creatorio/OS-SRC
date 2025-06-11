BUILD_DIR?=build/
ASM?=nasm
SOURCES_C=$(wildcard *.c)
OBJECTS_C=$(patsubst %.c, $(BUILD_DIR)/%.o, $(SOURCES_C))

.PHONY: all clean

all: UEFI

UEFI: $(BUILD_DIR)/BOOTX64.EFI

#$(OBJECTS_C): $(SOURCES_C)
# compile: (flags before -o become CFLAGS in the Makefile)
#	 -c -o $(BUILD_DIR)/$<.o $<
$(BUILD_DIR)/BOOTX64.EFI: $(OBJECTS_C)
# link: (flags before -o become LDFLAGS in the Makefile)
	@x86_64-w64-mingw32-gcc -nostdlib -Wl,--subsystem,10 -e efi_main -o $(BUILD_DIR)/BOOTX64.EFI $(OBJECTS_C)
	@echo "--> Created BOOTX64.EFI"
$(BUILD_DIR)/%.o: %.c
	@x86_64-w64-mingw32-gcc -ffreestanding -mno-red-zone -Wall -Wextra -Wpedantic -MMD -std=c17 -c -o $@ $<
	@echo "--> Compiled: " $<


clean:
	@rm -f $(BUILD_DIR)/BOOTX64.EFI