#ifndef _MY_HEADER_H
#define _MY_HEADER_H hdr
#include "efi.h"
// #include "/home/osmaker/gnu-efi-3.0.18/inc/efi.h"
// #include "UndefaultHeaders.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#pragma once
EFI_INPUT_KEY get_key(void);
void INIT(EFI_SYSTEM_TABLE *ST, EFI_HANDLE ImageHandle);
bool printf(CHAR16 *fmt, ...);
bool error(CHAR16 *fmt, ...);

extern EFI_STATUS test_mouse(void);
extern EFI_STATUS set_text_mode(void);
extern EFI_STATUS set_graphics_mode(void);
extern EFI_STATUS read_file_ESP(void);
extern EFI_STATUS print_block_io_partitions(void);
extern EFI_STATUS print_memory_map(void);
extern EFI_STATUS print_configuration_table(void);
extern EFI_STATUS print_ACPI_table(void);
extern EFI_STATUS load_kernel(void);
extern VOID print_datetime(IN EFI_EVENT event, IN VOID *Context);
extern EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *cout;
extern EFI_SIMPLE_TEXT_INPUT_PROTOCOL *cin;
extern EFI_HANDLE ih;
extern EFI_SYSTEM_TABLE *st;
extern EFI_BOOT_SERVICES *bs;
extern EFI_RUNTIME_SERVICES *rs;
extern EFI_EVENT timer_event;

// -----------------
// Global constants
// -----------------
#define PAGE_SIZE 4096 // 4KiB
#define PHYS_PAGE_ADDRESS_MASK 0x000FFFFFFFFFF000
#define KERNEL_START_ADDRESS 0xFFFFFFFF80000000
#define STACK_PAGES 16

// ELF Header - x86_64
typedef struct
{
    struct
    {
        UINT8 ei_mag0;
        UINT8 ei_mag1;
        UINT8 ei_mag2;
        UINT8 ei_mag3;
        UINT8 ei_class;
        UINT8 ei_data;
        UINT8 ei_version;
        UINT8 ei_osabi;
        UINT8 ei_abiversion;
        UINT8 ei_pad[7];
    } e_ident;

    UINT16 e_type;
    UINT16 e_machine;
    UINT32 e_version;
    UINT64 e_entry;
    UINT64 e_phoff;
    UINT64 e_shoff;
    UINT32 e_flags;
    UINT16 e_ehsize;
    UINT16 e_phentsize;
    UINT16 e_phnum;
    UINT16 e_shentsize;
    UINT16 e_shnum;
    UINT16 e_shstrndx;
} __attribute__((packed)) ELF_Header_64;

// ELF Program Header - x86_64
typedef struct
{
    UINT32 p_type;
    UINT32 p_flags;
    UINT64 p_offset;
    UINT64 p_vaddr;
    UINT64 p_paddr;
    UINT64 p_filesz;
    UINT64 p_memsz;
    UINT64 p_align;
} __attribute__((packed)) ELF_Program_Header_64;

// Elf Header e_type values
typedef enum
{
    ET_EXEC = 0x2,
    ET_DYN = 0x3,
} ELF_EHEADER_TYPE;

// Elf Program header p_type values
typedef enum
{
    PT_NULL = 0x0,
    PT_LOAD = 0x1, // Loadable
} ELF_PHEADER_TYPE;

// PE Structs/types
// PE32+ COFF File Header
typedef struct
{
    UINT16 Machine;          // 0x8664 = x86_64
    UINT16 NumberOfSections; // # of sections to load for program
    UINT32 TimeDateStamp;
    UINT32 PointerToSymbolTable;
    UINT32 NumberOfSymbols;
    UINT16 SizeOfOptionalHeader;
    UINT16 Characteristics;
} __attribute__((packed)) PE_Coff_File_Header_64;

// COFF File Header Characteristics
typedef enum
{
    IMAGE_FILE_EXECUTABLE_IMAGE = 0x0002,
} PE_COFF_CHARACTERISTICS;

// PE32+ Optional Header
typedef struct
{
    UINT16 Magic; // 0x10B = PE32, 0x20B = PE32+
    UINT8 MajorLinkerVersion;
    UINT8 MinorLinkerVersion;
    UINT32 SizeOfCode;
    UINT32 SizeOfInitializedData;
    UINT32 SizeOfUninitializedData;
    UINT32 AddressOfEntryPoint; // Entry Point address (RVA from image base in memory)
    UINT32 BaseOfCode;
    UINT64 ImageBase;
    UINT32 SectionAlignment;
    UINT32 FileAlignment;
    UINT16 MajorOperatingSystemVersion;
    UINT16 MinorOperatingSystemVersion;
    UINT16 MajorImageVersion;
    UINT16 MinorImageVersion;
    UINT16 MajorSubsystemVersion;
    UINT16 MinorSubsystemVersion;
    UINT32 Win32VersionValue;
    UINT32 SizeOfImage; // Size in bytes of entire file (image) incl. headers
    UINT32 SizeOfHeaders;
    UINT32 CheckSum;
    UINT16 Subsystem;
    UINT16 DllCharacteristics;
    UINT64 SizeOfStackReserve;
    UINT64 SizeOfStackCommit;
    UINT64 SizeOfHeapReserve;
    UINT64 SizeOfHeapCommit;
    UINT32 LoaderFlags;
    UINT32 NumberOfRvaAndSizes;
} __attribute__((packed)) PE_Optional_Header_64;

// Optional Header DllCharacteristics
typedef enum
{
    IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE = 0x0040, // PIE executable
} PE_OPTIONAL_HEADER_DLLCHARACTERISTICS;

// PE32+ Section Headers - Immediately following the Optional Header
typedef struct
{
    UINT64 Name;             // 8 byte, null padded UTF-8 string
    UINT32 VirtualSize;      // Size in memory; If >SizeOfRawData, the difference is 0-padded
    UINT32 VirtualAddress;   // RVA from image base in memory
    UINT32 SizeOfRawData;    // Size of actual data (similar to ELF FileSize)
    UINT32 PointerToRawData; // Address of actual data
    UINT32 PointerToRelocations;
    UINT32 PointerToLinenumbers;
    UINT16 NumberOfRelocations;
    UINT16 NumberOfLinenumbers;
    UINT32 Characteristics;
} __attribute__((packed)) PE_Section_Header_64;

typedef struct
{
    UINTN size;
    EFI_MEMORY_DESCRIPTOR *map;
    UINTN key;
    UINTN desc_size;
    UINT32 desc_version;
} Memory_Map_Info;

typedef struct
{
    Memory_Map_Info mmap;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE gop_mode;
    EFI_RUNTIME_SERVICES *RuntimeServices;
    EFI_CONFIGURATION_TABLE *ConfigurationTable;
    UINTN NumberOfTableEntries;
} Kernel_Parms;

typedef void EFIAPI (*Entry_Point)(Kernel_Parms*);

typedef struct
{
    EFI_GUID guid;
    char16_t *string;
} EGWS;

typedef struct
{
    UINT8 signature[4];
    UINT32 length;
    UINT8 revision;
    UINT8 checksum;
    UINT8 QEMID[6];
    UINT8 QEM_table_id[8];
    UINT32 QEM_revision;
    UINT32 creator_id;
    UINT32 creator_revision;
} __attribute__((packed)) ACPI_Description_Header;

extern EGWS CTGAS[6];

typedef struct
{
    UINTN cols;
    UINTN rows;
} screen_bounds;

enum
{
    PRESENT = (1 << 0),
    READWRITE = (1 << 1),
    USER = (1 << 2),
};

typedef struct
{
    UINT64 entries[512];
} Page_Table;
extern Page_Table *pml4;

// TODO: Types for descriptors: GDT, TSS, TSS DESC

typedef struct
{
    UINT16 limit;
    UINT64 base;
} __attribute__((packed)) Descriptor_Register;

typedef struct
{
    union
    {
        UINT64 value;
        struct
        {
            UINT64 limit_15_0 : 16;
            UINT64 base_15_0 : 16;
            UINT64 base_23_16 : 8;
            UINT64 type : 4;
            UINT64 s : 1;
            UINT64 dpl : 2;
            UINT64 p : 1;
            UINT64 limit_19_16 : 4;
            UINT64 avl : 1;
            UINT64 l : 1;
            UINT64 d_b : 1;
            UINT64 g : 1;
            UINT64 base_31_24 : 8;
        };
    };

} __attribute__((packed)) x86_64_Descriptor;

typedef struct
{
    x86_64_Descriptor descriptor;
    UINT32 base_63_32;
    UINT32 zero;
} __attribute__((packed)) TSS_LDT_Descriptor;

typedef struct
{
    UINT32 reserved_1;
    UINT32 RSP0_lower;
    UINT32 RSP0_upper;
    UINT32 RSP1_lower;
    UINT32 RSP1_upper;
    UINT32 RSP2_lower;
    UINT32 RSP2_upper;
    UINT32 reserved_2;
    UINT32 reserved_3;
    UINT32 IST1_lower;
    UINT32 IST1_upper;
    UINT32 IST2_lower;
    UINT32 IST2_upper;
    UINT32 IST3_lower;
    UINT32 IST3_upper;
    UINT32 IST4_lower;
    UINT32 IST4_upper;
    UINT32 IST5_lower;
    UINT32 IST5_upper;
    UINT32 IST6_lower;
    UINT32 IST6_upper;
    UINT32 IST7_lower;
    UINT32 IST7_upper;
    UINT32 reserved_4;
    UINT32 reserved_5;
    UINT16 reserved_6;
    UINT16 io_map_base;
} __attribute__((packed)) TSS;

typedef struct
{
    x86_64_Descriptor null;
    x86_64_Descriptor kernel_code_64;
    x86_64_Descriptor kernel_data_64;
    x86_64_Descriptor user_code_64;
    x86_64_Descriptor user_data_64;
    x86_64_Descriptor kernel_code_32;
    x86_64_Descriptor kernel_data_32;
    x86_64_Descriptor user_code_32;
    x86_64_Descriptor user_data_32;
    TSS_LDT_Descriptor tss;
} GDT;

CHAR16 *
strcpy_u16(CHAR16 *dst, const CHAR16 *src);
INTN strncmp_u16(CHAR16 *s1, CHAR16 *s2, UINTN len);
CHAR16 *strrchr_u16(CHAR16 *str, CHAR16 c);
CHAR16 *strcat_u16(CHAR16 *dst, CHAR16 *src);
VOID *memset(VOID *dst, UINT8 c, UINTN len);
VOID *memcpy(VOID *dst, VOID *src, UINTN len);
INTN memcmp(VOID *m1, VOID *m2, UINTN len);
UINTN strlen(char *s);
bool isdigit(char c);
char *strstr(char *haystack, char *needle);
INTN strcmp(char *s1, char *s2);
EFI_STATUS get_memory_map(Memory_Map_Info *mmap);
VOID *get_config_table_by_guid(EFI_GUID guid);
#define px_LGRAY 0xee, 0xee, 0xee, 0x00
#define px_BLACK 0x00, 0x00, 0x00, 0x00
#define px_EFIBLUE 0x98, 0x00, 0x00, 0x00
#define ESC 0x17
#define UP_ARROW 0x1
#define DOWN_ARROW 0x2
#define DEFAULT_FG_COLOR EFI_YELLOW
#define DEFAULT_BG_COLOR EFI_BLUE
#define HIGHLIGHT_FG_COLOR EFI_BLUE
#define HIGHLIGHT_BG_COLOR EFI_CYAN
#define ArraySize(x) (sizeof(x) / sizeof(x[0]))
#endif
