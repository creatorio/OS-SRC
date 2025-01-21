#include <efi.h>
#include <efilib.h>
#include <stdbool.h>
#pragma once
EFI_INPUT_KEY get_key(void);
void INIT(EFI_SYSTEM_TABLE *ST, EFI_HANDLE ImageHandle);
bool printf(CHAR16 *fmt, ...);
EFI_STATUS test_mouse(void);
EFI_STATUS set_text_mode(void);
EFI_STATUS set_graphics_mode(void);
EFI_STATUS read_file_ESP(void);
VOID print_datetime(IN EFI_EVENT event, IN VOID *Context);
extern EFI_SIMPLE_TEXT_OUT_PROTOCOL *cout;
extern EFI_SIMPLE_TEXT_IN_PROTOCOL *cin;
extern EFI_HANDLE ih;
extern EFI_SYSTEM_TABLE *st;
extern EFI_BOOT_SERVICES *bs;
extern EFI_RUNTIME_SERVICES *rs;
typedef struct
{
    UINTN cols;
    UINTN rows
} screen_bounds;
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