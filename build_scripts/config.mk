export CFLAGS =-std=c17 -MMD -Wall -Wextra -Wpedantic -mno-red-zone -ffreestanding
export ASMFLAGS =
export CC = clang
export CXX = g++
export LD = gcc
export ASM = nasm
export LINKFLAGS =
export LIBS =

export TARGET = /home/osmaker/TOOLCHAIN_i686-elf/bin/i686-elf
export TARGET64 = /home/osmaker/TOOLCHAIN_x86_64-elf/bin/x86_64-elf
export TARGET_ASM = nasm
export TARGET_ASMFLAGS =
export TARGET_CFLAGS = -std=c99 -g #-O2
export TARGET_CC = $(TARGET)-gcc
export TARGET_CC64 = $(TARGET64)-gcc
export TARGET_CXX = $(TARGET)-g++
export TARGET_CXX64 = $(TARGET64)-g++
export TARGET_LD = $(TARGET)-gcc
export TARGET_LD64 = $(TARGET64)-gcc
export TARGET_LINKFLAGS =
export TARGET_LIBS =

export BUILD_DIR = $(abspath build)

BINUTILS_VERSION = 2.37
BINUTILS_URL = https://ftp.gnu.org/gnu/binutils/binutils-$(BINUTILS_VERSION).tar.xz

GCC_VERSION = 11.2.0
GCC_URL = https://ftp.gnu.org/gnu/gcc/gcc-$(GCC_VERSION)/gcc-$(GCC_VERSION).tar.xz