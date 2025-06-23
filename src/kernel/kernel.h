#ifndef _MY_HEADER_H2
#define _MY_HEADER_H2 hdr2
#include "../bootloader/uefilib.h"
#pragma once
extern UINT8 font8x16[128][16][8];

extern BOOLEAN add_int_to_buf_c16(UINTN number, UINT8 base, BOOLEAN signed_num, UINTN min_digits, CHAR16 *buf, UINTN *buf_idx);
extern CHAR16 *strrev_c16(CHAR16 *s);
extern UINTN strlen_c16(CHAR16 *s);
extern CHAR16 *strcpy_c16(CHAR16 *dst, CHAR16 *src);
extern CHAR16 *strcat_c16(CHAR16 *dst, CHAR16 *src);
#endif
