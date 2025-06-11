#include "../bootloader/uefilib.h"

__attribute__((section(".kernel"), aligned(0x1000))) void EFIAPI kmain(Kernel_Parms *kargs)
{
    EFI_STATUS status;
    UINT32 *fb = (UINT32 *)kargs->gop_mode.FrameBufferBase;
    UINT32 xres = kargs->gop_mode.Info->HorizontalResolution;
    UINT32 yres = kargs->gop_mode.Info->VerticalResolution;

    for (UINT32 y = 0; y < yres; y++)
    {
        for (UINT32 x = 0; x < xres; x++)
        {
            fb[y * xres + x] = 0xFFDDDDDD;
        }
    }

    for (UINT32 y = 0; y < yres / 5; y++)
    {
        for (UINT32 x = 0; x < xres / 5; x++)
        {
            fb[y * xres + x] = 0xFFCC2222;
        }
    }

    EFI_TIME old_time, new_time;
    EFI_TIME_CAPABILITIES time_cap;
    UINTN i = 0;
    while (i < 10)
    {
        status = kargs->RuntimeServices->GetTime(&new_time, &time_cap);
        if (EFI_ERROR(status))
        {
            for (UINT32 y = 0; y < yres / 5; y++)
            {
                for (UINT32 x = 0; x < xres / 5; x++)
                {
                    fb[y * xres + x] = 0x0000FF00;
                }
            }
            i++;
        }
        if (old_time.Second != new_time.Second)
        {
            old_time.Second = new_time.Second;

            i++;
            for (UINT32 y = 0; y < yres / 5; y++)
            {
                for (UINT32 x = 0; x < xres / 5; x++)
                {
                    fb[y * xres + x] = 0x0000FFFF;
                }
            }
        }
    }
    /*UINTN i = 0;
    while (i < 450000000)
    {
        i++;
    }*/
    kargs->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
}
