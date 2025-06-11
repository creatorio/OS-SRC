include build_scripts/config.mk
include build_scripts/toolchain.mk

KERNEL?=kernel.elf

USB: /dev/sdb
/dev/sdb: bootloader kernel
	@sudo sgdisk -o /dev/sdb
	@sudo sgdisk -n 1:0:93750 -t 1:ef00 /dev/sdb
	@sudo sgdisk -n 2:0:0 -t 2:0700 /dev/sdb
	@sudo mkfs.fat -F 32 /dev/sdb1
	@sudo mount /dev/sdb1 /mnt
	@sudo mkdir /mnt/EFI
	@sudo mkdir /mnt/EFI/BOOT
	@sudo cp $(BUILD_DIR)/BOOTX64.EFI /mnt/EFI/BOOT
	@sudo cp $(BUILD_DIR)/kernel.bin  /mnt/
	@sudo cp ./test.txt  /mnt/
	@sudo umount /mnt
	@echo "--> Formatted: USB Hardware Boot"

#
# Floppy image
#
SPP: $(BUILD_DIR)/uefiSPP.img

$(BUILD_DIR)/uefiSPP.img: bootloader kernel
	@$ dd if=/dev/zero of=$(BUILD_DIR)/uefiSPP.img bs=512 count=131056
	@sgdisk -o $(BUILD_DIR)/uefiSPP.img
	@sgdisk -n 1:0:93750 -t 1:ef00 $(BUILD_DIR)/uefiSPP.img
	@sgdisk -n 2:0:0 -t 2:0700 $(BUILD_DIR)/uefiSPP.img
	@sgdisk -p
	@sudo losetup -Pf $(BUILD_DIR)/uefiSPP.img
	@sudo mkfs.fat -F 32 /dev/loop0p1
	@sudo mount /dev/loop0p1 /mnt
	@sudo mkdir /mnt/EFI
	@sudo mkdir /mnt/EFI/BOOT
	@sudo cp $(BUILD_DIR)/BOOTX64.EFI /mnt/EFI/BOOT
	@sudo cp $(BUILD_DIR)/kernel.bin  /mnt/
	@sudo cp ./test.txt  /mnt/
	@sudo umount /mnt
	@sudo losetup -d /dev/loop0
	@echo "--> Created: " $@
	@echo "--> Emulating: Hard Disk Boot With Qemu Uefi SPP"
	@qemu-system-x86_64 -drive format=raw,file=./build/uefiSPP.img -bios /home/osmaker/Downloads/bios64.bin -m 256M -vga std -name TESTOS -machine q35 -usb -device usb-mouse -rtc base=localtime 

APP: $(BUILD_DIR)/uefiAPP.img

$(BUILD_DIR)/uefiAPP.img: bootloader kernel
	@$ dd if=/dev/zero of=$(BUILD_DIR)/uefiAPP.img bs=512 count=131056
	@sgdisk -o $(BUILD_DIR)/uefiAPP.img
	@sgdisk -n 1:0:93750 -t 1:ef00 $(BUILD_DIR)/uefiAPP.img
	@sgdisk -n 2:0:0 -t 2:0700 $(BUILD_DIR)/uefiAPP.img
	@sgdisk -p
	@sudo losetup -Pf $(BUILD_DIR)/uefiAPP.img
	@sudo mkfs.fat -F 32 /dev/loop0p1
	@sudo mount /dev/loop0p1 /mnt
	@sudo mkdir /mnt/EFI
	@sudo mkdir /mnt/EFI/BOOT
	@sudo cp $(BUILD_DIR)/BOOTX64.EFI /mnt/EFI/BOOT
	@sudo cp $(BUILD_DIR)/kernel.bin  /mnt/
	@sudo cp $(BUILD_DIR)/test.txt  /mnt/
	@sudo umount /mnt
	@sudo losetup -d /dev/loop0
	@echo "--> Created: " $@
	@echo "--> Emulating: Hard Disk Boot With Qemu Uefi APP"
	@qemu-system-x86_64 -drive format=raw,file=./build/uefiAPP.img -bios /home/osmaker/Downloads/bios64_app.bin -m 256M -vga std -name TESTOS -machine q35 -usb -device usb-mouse -rtc base=localtime 

QM: $(BUILD_DIR)/uefiQM.img
$(BUILD_DIR)/uefiQM.img: bootloader kernel
	@sudo /home/osmaker/Downloads/UEFI-GPT-image-creator-a77b0e6655459221f71f131500275e725b40cfbb/write_gpt -ae /EFI/BOOT/ ./build/BOOTX64.EFI / ./testefi.txt -ad ./build/$(KERNEL) -ds 5 -es 35 -i ./build/uefiQM.img
	@echo "--> Emulating: Hard Disk Boot With Qemu Uefi SPP using QM"
	@sudo qemu-system-x86_64 -gdb tcp::1234 -drive format=raw,file=./build/uefiQM.img -bios /home/osmaker/Downloads/bios64.bin -m 256M -vga std -name TESTOS -machine q35 -usb -device usb-mouse -rtc base=localtime 
#
# Bootloader
#
bootloader: stage1

stage1: $(BUILD_DIR)/stage1.bin

$(BUILD_DIR)/stage1.bin: always
	@$(MAKE) --no-print-directory -C src/bootloader BUILD_DIR=$(abspath $(BUILD_DIR))

#
# Kernel
#
kernel: $(BUILD_DIR)/kernel.bin

$(BUILD_DIR)/kernel.bin: always
	@$(MAKE) --no-print-directory -C src/kernel BUILD_DIR=$(abspath $(BUILD_DIR))

#
# Always
#
always:
	@mkdir -p $(BUILD_DIR)


# Clean
#
clean:
	@echo "--> Deleting: kernel.bin"
	@echo "--> Deleting: kernel.elf"
	@echo "--> Deleting: kernel.pe"
	@echo "--> Deleting: BOOTX64.EFI"
	@echo "--> Clearing: Build Directory"
	@$(MAKE) --no-print-directory -C src/bootloader BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	@$(MAKE) --no-print-directory -C src/kernel BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	@rm -rf $(BUILD_DIR)/*
