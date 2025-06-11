.PHONY: all clean

all: kernelbin kernelelf kernelbinpe kernelpe

kernelbin: $(BUILD_DIR)/kernel.bin
$(BUILD_DIR)/kernel.bin: kernel.c
	@sudo clang -c $(CFLAGS) -fPIE -o $(BUILD_DIR)/kernel.o ./kernel.c
	@echo "--> Compiled Kernel.c"
	@sudo ld -nostdlib -pie -e kmain --oformat binary -o $(BUILD_DIR)/kernel.bin $(BUILD_DIR)/kernel.o 
	@echo "--> Created Kernel.bin"

kernelelf: $(BUILD_DIR)/kernel.elf
$(BUILD_DIR)/kernel.elf: kernel.c
	sudo clang $(CFLAGS) -fPIE -e kmain -nostdlib -o $(BUILD_DIR)/kernel.elf ./kernel.c

kernelbinpe: $(BUILD_DIR)/kernel.binpe
$(BUILD_DIR)/kernel.binpe: kernel.c
	sudo x86_64-w64-mingw32-gcc -c $(CFLAGS) -fPIE -o $(BUILD_DIR)/kernel.o ./kernel.c
	sudo x86_64-w64-mingw32-ld -pie -nostdlib -e kmain -o $(BUILD_DIR)/kernel.obj $(BUILD_DIR)/kernel.o 
	objcopy -O binary $(BUILD_DIR)/kernel.obj $@

kernelpe: $(BUILD_DIR)/kernel.pe
$(BUILD_DIR)/kernel.pe: kernel.c
	sudo x86_64-w64-mingw32-gcc $(CFLAGS) -fPIE -e kmain -nostdlib -o $(BUILD_DIR)/kernel.pe ./kernel.c


clean:
	@rm -f $(BUILD_DIR)/kernel.bin