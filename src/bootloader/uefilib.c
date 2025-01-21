#include "uefilib.h"
EFI_SIMPLE_TEXT_OUT_PROTOCOL *cout = NULL;
EFI_SIMPLE_TEXT_IN_PROTOCOL *cin = NULL;
EFI_HANDLE ih = NULL;
EFI_SYSTEM_TABLE *st = NULL;
EFI_BOOT_SERVICES *bs = NULL;
EFI_RUNTIME_SERVICES *rs = NULL;
EFI_GRAPHICS_OUTPUT_BLT_PIXEL cursor_buffer[] = {
    px_LGRAY, px_LGRAY, px_EFIBLUE, px_EFIBLUE, px_EFIBLUE, px_EFIBLUE, px_EFIBLUE, px_EFIBLUE,
    px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_EFIBLUE, px_EFIBLUE, px_EFIBLUE, px_EFIBLUE,
    px_EFIBLUE, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_EFIBLUE,
    px_EFIBLUE, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY,
    px_EFIBLUE, px_EFIBLUE, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_EFIBLUE,
    px_EFIBLUE, px_EFIBLUE, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_EFIBLUE, px_EFIBLUE,
    px_EFIBLUE, px_EFIBLUE, px_LGRAY, px_LGRAY, px_LGRAY, px_EFIBLUE, px_LGRAY, px_EFIBLUE,
    px_EFIBLUE, px_EFIBLUE, px_EFIBLUE, px_LGRAY, px_EFIBLUE, px_EFIBLUE, px_EFIBLUE, px_LGRAY

};
void INIT(EFI_SYSTEM_TABLE *ST, EFI_HANDLE ImageHandl)
{
    st = ST;
    bs = ST->BootServices;
    rs = ST->RuntimeServices;
    cout = ST->ConOut;
    cin = ST->ConIn;
    ih = ImageHandl;
}

EFI_STATUS read_file_ESP(void)
{
    EFI_GUID lg = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *l;
    EFI_GUID sfspg = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfsp;
    EFI_STATUS status = EFI_SUCCESS;

    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));

    cout->ClearScreen(cout);

    status = bs->OpenProtocol(ih, &lg, (VOID **)&l, ih, NULL, ByProtocol);
    if (EFI_ERROR(status))
    {
        printf(u"  ERROR: %x\r\nCould not locate Loaded Image Protocol handle buffer", status);
        goto cleanup;
    }
    status = bs->OpenProtocol(l->DeviceHandle, &sfspg, (VOID **)&sfsp, ih, NULL, ByProtocol);
    if (EFI_ERROR(status))
    {
        printf(u"  ERROR: %x\r\nCould not locate Simple Filesystem Protocol handle buffer", status);
        goto cleanup;
    }
    EFI_FILE_PROTOCOL *dirp = NULL;
    status = sfsp->OpenVolume(sfsp, &dirp);

    if (EFI_ERROR(status))
    {
        printf(u"  ERROR: %x\r\nCould not Open Root", status);
        goto cleanup;
    }
    CHAR16 CURDIR[256];
    strcpy(CURDIR, u"/");
    EFI_FILE_INFO file_info;
    UINTN buf_size = sizeof(file_info);
    dirp->Read(dirp, &buf_size, &file_info);
    while (buf_size > 0)
    {
        dirp->Read(dirp, &buf_size, &file_info);
    }

    get_key();
cleanup:
    bs->CloseProtocol(ih, &lg, ih, NULL);
    bs->CloseProtocol(l->DeviceHandle, &sfspg, ih, NULL);
    return status;
}

VOID EFIAPI print_datetime(IN EFI_EVENT event, IN VOID *Context)
{
    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
    (void)event;
    screen_bounds context = *(screen_bounds *)Context;
    UINTN sc = cout->Mode->CursorColumn, sr = cout->Mode->CursorRow;
    cout->SetCursorPosition(cout, context.cols - 32, context.rows - 1);
    EFI_TIME time = {0};
    EFI_TIME_CAPABILITIES capabilities = {0};
    rs->GetTime(&time, &capabilities);
    printf(u"Date: %d/%c%d/%c%d Time: %c%d:%c%d:%c%d",
           time.Year,
           time.Month < 10 ? u'0' : u'\0', time.Month,
           time.Day < 10 ? u'0' : u'\0', time.Day,
           time.Hour < 10 ? u'0' : u'\0', time.Hour,
           time.Minute < 10 ? u'0' : u'\0', time.Minute,
           time.Second < 10 ? u'0' : u'\0', time.Second);
    cout->SetCursorPosition(cout, sc, sr);
}

EFI_STATUS test_mouse(void)
{
    EFI_STATUS status = 0;
    EFI_GUID spp_guid = EFI_SIMPLE_POINTER_PROTOCOL_GUID;
    EFI_SIMPLE_POINTER_PROTOCOL *spp[5] = {0};
    EFI_GUID app_guid = EFI_ABSOLUTE_POINTER_PROTOCOL_GUID;
    EFI_ABSOLUTE_POINTER_PROTOCOL *app[5] = {0};
    EFI_ABSOLUTE_POINTER_MODE *spp_mode = NULL;
    UINTN num_handles = 0;
    EFI_HANDLE *handle_buffer = NULL;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *mode_info = NULL;
    UINTN mode_size = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
    UINTN cursor_size = 8;
    INTN cursorx = 0, cursory = 0;

    typedef enum
    {
        CIN = 0,
        SPP = 1,
        APP = 2,
    } INPUT_TYPE;
    typedef struct
    {
        EFI_EVENT wait_event;
        INPUT_TYPE type;
        union
        {
            EFI_SIMPLE_POINTER_PROTOCOL *spp;
            EFI_ABSOLUTE_POINTER_PROTOCOL *app;
        }
    } INPUT_PROTOCOLS;

    INPUT_PROTOCOLS input_protocols[11] = {0};
    UINTN num_protocols = 0;

    input_protocols[num_protocols++] = (INPUT_PROTOCOLS){
        .wait_event = cin->WaitForKey,
        .type = CIN,
        .spp = NULL};

    EFI_GRAPHICS_OUTPUT_BLT_PIXEL save_buffer[8 * 8];

    status = bs->LocateProtocol(&gop_guid, NULL, (VOID **)&gop);

    status = gop->QueryMode(gop, gop->Mode->Mode, &mode_size, &mode_info);

    status = bs->LocateHandleBuffer(ByProtocol, &spp_guid, NULL, &num_handles, &handle_buffer);

    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));

    cout->ClearScreen(cout);

    if (EFI_ERROR(status))
    {
        printf(u"  ERROR: %x\r\nCould not locate Simple Pointer Protocol handle buffer", status);
    }

    cout->ClearScreen(cout);
    bool mode = 0;
    for (UINTN i = 0; i < num_handles; i++)
    {
        status = bs->OpenProtocol(handle_buffer[i], &spp_guid, (VOID **)&spp[i], ih, NULL, ByProtocol);
        if (EFI_ERROR(status))
        {
            printf(u"  ERROR: %x\r\nCould not locate Simple Pointer Protocol handle buffer", status);
            continue;
        }

        spp[i]->Reset(spp[i], true);

        printf(u"SPP: %d\r\nResolution X: %u\r\nResolution Y: %u\r\nResolution Z: %u\r\nLButton: %b\r\nRButton: %b\r\n",
               i,
               spp[i]->Mode->ResolutionX,
               spp[i]->Mode->ResolutionY,
               spp[i]->Mode->ResolutionZ,
               spp[i]->Mode->LeftButton,
               spp[i]->Mode->RightButton);
        if (spp[i]->Mode->ResolutionX < 65536)
        {
            mode = 1;
            input_protocols[num_protocols++] = (INPUT_PROTOCOLS){
                .wait_event = spp[i]->WaitForInput,
                .type = SPP,
                .spp = spp[i]};
        }
    }

    if (!mode)
    {
        printf(u"  ERROR: Could not find Simple Pointer Protocol mode");
    }
    num_handles = 0;
    bs->FreePool((VOID *)handle_buffer);
    handle_buffer = NULL;
    mode = false;
    printf(u"\r\n");
    status = bs->LocateHandleBuffer(ByProtocol, &app_guid, NULL, &num_handles, &handle_buffer);
    if (EFI_ERROR(status))
    {
        printf(u"  ERROR: %x\r\nCould not locate Absolute Pointer Protocol handle buffer", status);
    }

    for (UINTN i = 0; i < num_handles; i++)
    {
        status = bs->OpenProtocol(handle_buffer[i], &app_guid, (VOID **)&app[i], ih, NULL, ByProtocol);
        if (EFI_ERROR(status))
        {
            printf(u"  ERROR: %x\r\nCould not locate Simple Pointer Protocol handle buffer", status);
            continue;
        }

        app[i]->Reset(app[i], true);
        printf(u"APP: %u\r\nMin X: %u\r\nMin Y: %u\r\nMin Z: %u\r\nMax X: %u\r\nMax Y: %u\r\nMax Z: %u\r\nAttributes: %b\r\n",
               i,
               app[i]->Mode->AbsoluteMinX,
               app[i]->Mode->AbsoluteMinY,
               app[i]->Mode->AbsoluteMinZ,
               app[i]->Mode->AbsoluteMaxX,
               app[i]->Mode->AbsoluteMaxY,
               app[i]->Mode->AbsoluteMaxZ,
               app[i]->Mode->Attributes);
        if (app[i]->Mode->AbsoluteMaxX < 65536)
        {
            mode = 1;
            input_protocols[num_protocols++] = (INPUT_PROTOCOLS){
                .wait_event = app[i]->WaitForInput,
                .type = APP,
                .app = app[i]};
        }
    }

    if (!mode)
    {
        printf(u"  ERROR: Could not find Absolute Pointer Protocol mode");
    }

    if (num_protocols == 0)
    {
        printf(u"  ERROR: Could not find Simple or Absolute Pointer Protocols\r\n");
        get_key();
        return 1;
    }

    printf(u"\r\n  Mouse Xmm: %d, Ymm: %d, LB: %d, RB: %d\r", 0, 0, 0, 0);
    INT32 xr, yr;
    cursorx = ((xr = mode_info->HorizontalResolution) / 2) - (cursor_size / 2);
    cursory = ((yr = mode_info->VerticalResolution) / 2) - (cursor_size / 2);
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL pixel = {0xee,
                                           0xee,
                                           0xee,
                                           0x00};
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *fb = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)gop->Mode->FrameBufferBase;
    for (UINTN x = 0; x < cursor_size; x++)
    {
        for (UINTN y = 0; y < cursor_size; y++)
        {
            save_buffer[(y * cursor_size) + x] =
                fb[(mode_info->PixelsPerScanLine * (cursory + y)) + cursorx + x];
            fb[(mode_info->PixelsPerScanLine * (cursory + y)) + cursorx + x] = cursor_buffer[(y * cursor_size) + x];
        }
    }
    EFI_EVENT events[11] = {0};

    for (UINTN i = 0; i < num_protocols; i++)
    {
        events[i] = input_protocols[i].wait_event;
    }

    while (1)
    {
        UINTN index = 0;
        bs->WaitForEvent(num_protocols, events, &index);
        // printf(u"  Debog %d", SPP);

        if (input_protocols[index].type == CIN)
        {
            EFI_INPUT_KEY key;
            cin->ReadKeyStroke(cin, &key);
            if (key.ScanCode == ESC)
            {
                rs->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
            }
            break;
        }
        else if (input_protocols[index].type == SPP)
        {
            EFI_SIMPLE_POINTER_STATE state = {0};
            EFI_SIMPLE_POINTER_PROTOCOL *active_spp = input_protocols[index].spp;
            active_spp->GetState(active_spp, &state);

            float xmm_float = (float)state.RelativeMovementX / (float)active_spp->Mode->ResolutionX, ymm_float = (float)state.RelativeMovementY / (float)active_spp->Mode->ResolutionY;
            if (state.RelativeMovementX > 0 && xmm_float == 0.0)
                xmm_float = 1.0;
            if (state.RelativeMovementY > 0 && ymm_float == 0.0)
                ymm_float = 1.0;

            printf(u"                                                                     \rMouse cursorx: %d cursory: %d Xmm: %d, Ymm: %d, LB: %b, RB: %b\r", cursorx, cursory, (INTN)xmm_float, (INTN)ymm_float, state.LeftButton, state.RightButton);
            float xres_mm_px = mode_info->HorizontalResolution * 0.02;
            float yres_mm_px = mode_info->VerticalResolution * 0.02;
            EFI_GRAPHICS_OUTPUT_BLT_PIXEL px = {0x98, 0x00, 0x00, 0x00};

            EFI_GRAPHICS_OUTPUT_BLT_PIXEL pixel = {0xee,
                                                   0xee,
                                                   0xee,
                                                   0x00};
            fb = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)gop->Mode->FrameBufferBase;

            for (UINTN x = 0; x < cursor_size; x++)
            {
                for (UINTN y = 0; y < cursor_size; y++)
                {

                    fb[(mode_info->PixelsPerScanLine * (cursory + y)) + cursorx + x] = save_buffer[(y * cursor_size) + x];
                }
            }
            cursorx += (INTN)(xres_mm_px * xmm_float / 3.0);
            cursory += (INTN)(yres_mm_px * ymm_float / 2.0);
            if (cursorx < 0)
                cursorx = 0;
            if (cursorx > xr - cursor_size)
                cursorx = xr - cursor_size;
            if (cursory < 0)
                cursory = 0;
            if (cursory > yr - cursor_size)
                cursory = yr - cursor_size;
            fb = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)gop->Mode->FrameBufferBase;
            for (UINTN x = 0; x < cursor_size; x++)
            {
                for (UINTN y = 0; y < cursor_size; y++)
                {
                    save_buffer[(y * cursor_size) + x] =
                        fb[(mode_info->PixelsPerScanLine * (cursory + y)) + cursorx + x];
                    fb[(mode_info->PixelsPerScanLine * (cursory + y)) + cursorx + x] = cursor_buffer[(y * cursor_size) + x];
                }
            }
        }
        else if (input_protocols[index].type == APP)
        {
            EFI_ABSOLUTE_POINTER_STATE state = {0};
            EFI_ABSOLUTE_POINTER_PROTOCOL *active_app = input_protocols[index].app;
            active_app->GetState(active_app, &state);

            printf(u"                                                                     \rPointer x: %u y: %u z: %u, Buttons: %b\r", state.CurrentX, state.CurrentY, state.CurrentZ, state.ActiveButtons);

            fb = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)gop->Mode->FrameBufferBase;

            for (UINTN x = 0; x < cursor_size; x++)
            {
                for (UINTN y = 0; y < cursor_size; y++)
                {

                    fb[(mode_info->PixelsPerScanLine * (cursory + y)) + cursorx + x] = save_buffer[(y * cursor_size) + x];
                }
            }

            float x_app_ratio = (float)mode_info->HorizontalResolution / (float)active_app->Mode->AbsoluteMaxX;
            float y_app_ratio = (float)mode_info->VerticalResolution / (float)active_app->Mode->AbsoluteMaxY;

            cursorx = (INTN)((float)state.CurrentX * x_app_ratio);
            cursory = (INTN)((float)state.CurrentY * y_app_ratio);
            if (cursorx < 0)
                cursorx = 0;
            if (cursorx > xr - cursor_size)
                cursorx = xr - cursor_size;
            if (cursory < 0)
                cursory = 0;
            if (cursory > yr - cursor_size)
                cursory = yr - cursor_size;
            fb = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)gop->Mode->FrameBufferBase;
            for (UINTN x = 0; x < cursor_size; x++)
            {
                for (UINTN y = 0; y < cursor_size; y++)
                {
                    save_buffer[(y * cursor_size) + x] =
                        fb[(mode_info->PixelsPerScanLine * (cursory + y)) + cursorx + x];
                    fb[(mode_info->PixelsPerScanLine * (cursory + y)) + cursorx + x] = cursor_buffer[(y * cursor_size) + x];
                }
            }
        }
    }
    return EFI_SUCCESS;
}
EFI_STATUS set_graphics_mode(void)
{
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *mode_info = NULL;
    UINTN mode_size = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
    EFI_STATUS status = 0;
    UINTN mode_index = 0;

    typedef struct
    {
        UINT32 width;
        UINT32 height;
    } Gop_Mode_Info;

    Gop_Mode_Info gop_modes[75];
    status = bs->LocateProtocol(&gop_guid, NULL, (VOID **)&gop);

    if (EFI_ERROR(status))
    {
        printf(u"  ERROR: %x\r\nCould not locate Graphics Output Protocol", status);
        return status;
    }

    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));

    cout->ClearScreen(cout);

    // printf(u"STATUS: %x\r\n", status);
    //  UINTN max_cols, max_rows;
    //  cout->QueryMode(cout, cout->Mode->Mode, &max_cols, &max_rows);

    printf(u"  Graphics Mode Information:\r\n");

    status = gop->QueryMode(gop, gop->Mode->Mode, &mode_size, &mode_info);
    if (EFI_ERROR(status))
    {
        printf(u"  ERROR: %x\r\nCould not query Graphics Output Protocol Mode %x", status, gop->Mode->Mode);
        return status;
    }

    printf(u"  Max Mode: %d\r\n"
           u"  Current Mode: %d\r\n"
           u"  Framebuffer Address: %x\r\n"
           u"  Framebuffer Size: %d\r\n"
           u"  Pixel Format: %d\r\n"
           u"  Pixels Per Scan Line: %u\r\n"
           u"  Width x Height: %u x %u\r\n",
           gop->Mode->MaxMode - 1,
           gop->Mode->Mode,
           gop->Mode->FrameBufferBase,
           gop->Mode->FrameBufferSize,
           mode_info->PixelFormat,
           mode_info->PixelsPerScanLine,
           mode_info->HorizontalResolution,
           mode_info->VerticalResolution);

    printf(u"\r\n  Available Graphic modes:");

    UINTN menu_top = cout->Mode->CursorRow, menu_bottom = 0, max_cols;
    cout->QueryMode(cout, cout->Mode->Mode, &max_cols, &menu_bottom);
    menu_bottom -= 5;
    UINTN menu_len = menu_bottom - menu_top;
    cout->SetCursorPosition(cout, 0, menu_bottom + 3);
    printf(u"  Up/Down Arrow = Move Cursor\r\n  Enter = Select\r\n  Escape = Shutdown");
    cout->SetCursorPosition(cout, 0, menu_top);

    const INT32 max = gop->Mode->MaxMode;
    if (max < menu_len)
    {
        menu_bottom = menu_top + max;
        menu_len = menu_bottom - menu_top - 1;
    }
    for (UINTN i = 0; i < ArraySize(gop_modes) && i < (UINTN)max; i++)
    {
        gop->QueryMode(gop, i, &mode_size, &mode_info);
        gop_modes[i].width = mode_info->HorizontalResolution;
        gop_modes[i].height = mode_info->VerticalResolution;
    }

    cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
    printf(u"  Mode: %d, %dx%d", 0, gop_modes[0].width, gop_modes[0].height);
    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
    for (UINTN i = 1; i < menu_len + 1; i++)
    {
        printf(u"\r\n  Mode: %d, %dx%d", i, gop_modes[i].width, gop_modes[i].height);
    }
    cout->SetCursorPosition(cout, 0, menu_top);
    bool getting_input = true;
    while (getting_input)
    {
        UINTN current_row = cout->Mode->CursorRow;

        CHAR16 k[2];
        EFI_INPUT_KEY key = get_key();
        k[0] = key.UnicodeChar;
        k[1] = u"\0";
        // printf(u"Scancode: %x\r\nUnicodeChar: %s\r\n", key.ScanCode, k);
        /*UINTN mode = key.UnicodeChar - u'0';
        if (mode >= 0 && mode <= max)
        {
            printf(u"%s\r\n", k);
            get_key();
            EFI_STATUS status = 1;
            UINTN modee = cout->Mode->Mode;
            status = cout->SetMode(cout, mode);
            if (EFI_ERROR(status))
            {
                if (status == EFI_DEVICE_ERROR)
                {
                    printf(u"ERROR: %x; DEVICE ERROR\r\n", status);
                    get_key();
                    cout->SetMode(cout, modee);
                }
                else if (status == EFI_UNSUPPORTED)
                {
                    printf(u"ERROR: %x; Mode number was not valid\r\n", status);
                    printf(u"Press any key to continue");
                    get_key();
                    cout->SetMode(cout, modee);
                }
            }
            break;
        }*/
        switch (key.ScanCode)
        {
        case UP_ARROW:
            if (current_row == menu_top && mode_index > 0)
            {
                // printf(u"%d", menu_len);
                cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                mode_index--;
                printf(u"                                       \r  Mode: %d, %ux%u\r\n", mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);
                cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                UINTN temp_mode = mode_index + 1;
                for (UINTN i = 0; i < menu_len; i++, temp_mode++)
                {
                    printf(u"                                       \r  Mode: %d, %ux%u\r\n", temp_mode, gop_modes[temp_mode].width, gop_modes[temp_mode].height);
                }
                cout->SetCursorPosition(cout, 0, menu_top);
            }
            else if (current_row - 1 >= menu_top)
            {
                cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                printf(u"                                       \r  Mode: %d, %ux%u\r", mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);
                mode_index--;
                current_row--;
                cout->SetCursorPosition(cout, 0, current_row);
                cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                printf(u"                                       \r  Mode: %d, %ux%u\r", mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);
                cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
            }
            break;
        case DOWN_ARROW:
            // NOTE: Max avaialable mode is max - 1 , Source:UEFI SPEC;
            if (current_row == menu_bottom && mode_index < max - 1)
            {
                cout->SetCursorPosition(cout, 0, menu_top);
                cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));

                mode_index -= menu_len - 1;
                for (UINTN i = 1; i <= menu_len; i++, mode_index++)
                {
                    printf(u"                                       \r  Mode: %d, %ux%u\r\n", mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);
                }
                cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                printf(u"                                       \r  Mode: %d, %ux%u\r", mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);
            }
            else if (current_row + 1 <= menu_bottom)
            {
                cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                printf(u"                                       \r  Mode: %d, %ux%u\r\n", mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);
                mode_index++;
                current_row++;
                cout->SetCursorPosition(cout, 0, current_row);
                cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                printf(u"                                       \r  Mode: %d, %ux%u\r", mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);
                cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
            }
            cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));

            break;
        case ESC:
            rs->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);

        default:
            if ((CHAR16)key.UnicodeChar == 0xd)
            {
                gop->QueryMode(gop, mode_index, &mode_size, &mode_info);
                gop->SetMode(gop, mode_index);
                EFI_GRAPHICS_OUTPUT_BLT_PIXEL px = {0x98, 0x00, 0x00, 0x00};
                gop->Blt(gop, &px, EfiBltVideoFill, 0, 0, 0, 0, mode_info->HorizontalResolution, mode_info->VerticalResolution, 0);
                getting_input = false;
                break;
            }
        }
    }
}
EFI_STATUS set_text_mode(void)
{
    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
    cout->ClearScreen(cout);
    UINTN max_cols, max_rows;
    UINTN mode_index = 0;
    cout->QueryMode(cout, cout->Mode->Mode, &max_cols, &max_rows);
    cout->SetCursorPosition(cout, 0, max_rows - 3);
    typedef struct
    {
        UINT32 width;
        UINT32 height;
    } Gop_Mode_Info;

    Gop_Mode_Info gop_modes[10];
    printf(u"  Up/Down Arrow = Move Cursor\r\n  Enter = Select\r\n  Escape = Shutdown");
    cout->SetCursorPosition(cout, 0, 0);
    printf(u"  Text Mode Information:\r\n");
    printf(u"  Max Mode: %u\r\n"
           u"  Current Mode: %u\r\n"
           u"  Attribute: %x\r\n"
           u"  Cursor Column: %u\r\n"
           u"  Cursor Row: %u\r\n"
           u"  Cursor Visible: %d\r\n"
           u"  Columns: %u\r\n"
           u"  Rows: %u\r\n",
           cout->Mode->MaxMode,
           cout->Mode->Mode,
           cout->Mode->Attribute,
           cout->Mode->CursorColumn,
           cout->Mode->CursorRow,
           cout->Mode->CursorVisible,
           max_cols,
           max_rows);

    printf(u"  Available text modes:\r\n");
    UINTN menu_top = cout->Mode->CursorRow, menu_bottom;
    cout->QueryMode(cout, cout->Mode->Mode, &max_cols, &menu_bottom);
    cout->SetCursorPosition(cout, 0, max_rows);
    menu_bottom = menu_top + cout->Mode->MaxMode;
    UINTN menu_len = menu_bottom - menu_top;
    printf(u"                   \r  Up/Down Arrow = Move Cursor\r\n  Enter = Select\r\n  Escape = Shutdown");
    cout->SetCursorPosition(cout, 0, menu_top);
    const INT32 max = cout->Mode->MaxMode;
    cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
    cout->QueryMode(cout, 0, &max_cols, &max_rows);
    printf(u"                                       \r  Mode: %d, %ux%u\r\n", 0, max_cols, max_rows);
    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
    for (UINTN i = 0; i < ArraySize(gop_modes) && i < (UINTN)max; i++)
    {
        cout->QueryMode(cout, i, &max_cols, &max_rows);
        gop_modes[i].width = max_cols;
        gop_modes[i].height = max_rows;
    }
    for (UINTN i = 1; i < menu_len; i++)
    {
        printf(u"                                       \r  Mode: %d, %ux%u\r\n", i, gop_modes[i].width, gop_modes[i].height);
    }
    cout->SetCursorPosition(cout, 0, menu_top);
    bool getting_input = true;
    while (getting_input)
    {
        UINTN current_row = cout->Mode->CursorRow;

        CHAR16 k[2];
        EFI_INPUT_KEY key = get_key();
        k[0] = key.UnicodeChar;
        k[1] = u"\0";
        // printf(u"Scancode: %x\r\nUnicodeChar: %s\r\n", key.ScanCode, k);
        /*UINTN mode = key.UnicodeChar - u'0';
        if (mode >= 0 && mode <= max)
        {
            printf(u"%s\r\n", k);
            get_key();
            EFI_STATUS status = 1;
            UINTN modee = cout->Mode->Mode;
            status = cout->SetMode(cout, mode);
            if (EFI_ERROR(status))
            {
                if (status == EFI_DEVICE_ERROR)
                {
                    printf(u"ERROR: %x; DEVICE ERROR\r\n", status);
                    get_key();
                    cout->SetMode(cout, modee);
                }
                else if (status == EFI_UNSUPPORTED)
                {
                    printf(u"ERROR: %x; Mode number was not valid\r\n", status);
                    printf(u"Press any key to continue");
                    get_key();
                    cout->SetMode(cout, modee);
                }
            }
            break;
        }*/
        switch (key.ScanCode)
        {
        case UP_ARROW:
            if (current_row - 1 >= menu_top)
            {
                cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                printf(u"                                       \r  Mode: %d, %ux%u\r", mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);
                mode_index--;
                current_row--;
                cout->SetCursorPosition(cout, 0, current_row);
                cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                printf(u"                                       \r  Mode: %d, %ux%u\r", mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);
                cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
            }
            break;
        case DOWN_ARROW:
            if (current_row + 1 < menu_bottom)
            {
                cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                printf(u"                                       \r  Mode: %d, %ux%u\r\n", mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);
                mode_index++;
                current_row++;
                cout->SetCursorPosition(cout, 0, current_row);
                cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                printf(u"                                       \r  Mode: %d, %ux%u\r", mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);
                cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
            }
            cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));

            break;
        case ESC:
            rs->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);

        default:
            if ((CHAR16)key.UnicodeChar == 0xd)
            {
                EFI_STATUS status = 1;
                UINTN modee = cout->Mode->Mode;
                status = cout->SetMode(cout, mode_index);
                if (EFI_ERROR(status))
                {
                    if (status == EFI_DEVICE_ERROR)
                    {
                        cout->ClearScreen(cout);
                        printf(u"  ERROR: %x; DEVICE ERROR\r\n", status);
                        get_key();
                        cout->SetMode(cout, modee);
                    }
                    else if (status == EFI_UNSUPPORTED)
                    {
                        cout->ClearScreen(cout);
                        printf(u"  ERROR: %x; Mode number was not valid\r\n", status);
                        printf(u"  Press any key to continue");
                        get_key();
                        cout->SetMode(cout, modee);
                    }
                }

                getting_input = false;
                break;
            }
        }
    }
}

EFI_INPUT_KEY get_key(void)
{
    EFI_INPUT_KEY key;
    EFI_EVENT events[1];
    events[0] = cin->WaitForKey;
    UINTN index = 0;
    bs->WaitForEvent(1, events, &index);
    if (index == 0)
    {
        cin->ReadKeyStroke(cin, &key);
        return key;
    }
    return key;
}

bool print_number(UINTN number, UINT8 base, bool is_signed)
{

    const CHAR16 *digits = u"0123456789ABCDEF";
    CHAR16 buffer[24]; // Hopefully enough for UINTN_MAX (UINT64_MAX) + sign character
    UINTN i = 0;
    BOOLEAN negative = FALSE;

    if (base > 16)
    {
        cout->OutputString(cout, u"  Invalid base specified!\r\n");
        return FALSE; // Invalid base
    }

    // Only use and print negative numbers if decimal and signed True
    if (base == 10 && is_signed && (INTN)number < 0)
    {
        number = -(INTN)number; // Get absolute value of correct signed value to get digits to print
        negative = TRUE;
    }

    do
    {
        buffer[i++] = digits[number % base];
        number /= base;
    } while (number > 0);

    // Print negative sign for decimal numbers
    if (base == 10 && negative)
        buffer[i++] = u'-';
    if (base == 2)
    {
        buffer[i++] = u'b';
        buffer[i++] = u'0';
    }
    if (base == 8)
    {
        buffer[i++] = u'o';
        buffer[i++] = u'0';
    }
    if (base == 16)
    {
        buffer[i++] = u'x';
        buffer[i++] = u'0';
    }

    // NULL terminate string
    buffer[i--] = u'\0';

    // Reverse buffer before printing
    for (UINTN j = 0; j < i; j++, i--)
    {
        // Swap digits
        UINTN temp = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = temp;
    }

    cout->OutputString(cout, buffer);
    return TRUE;
}

bool printf(CHAR16 *fmt, ...)
{
    bool result;
    CHAR16 str[2];
    va_list args;
    va_start(args, fmt);

    str[0] = u'\0', str[1] = u'\0';
    for (UINTN i = 0; fmt[i] != u'\0'; i++)
    {
        if (fmt[i] == u'%')
        {
            i++;
            switch (fmt[i])
            {
            case u'c':
            {
                str[0] = va_arg(args, int);
                cout->OutputString(cout, str);
            }
            break;
            case u's':
            {
                CHAR16 *string = va_arg(args, CHAR16 *);
                cout->OutputString(cout, string);
            }
            break;
            case u'u':
            {
                UINT32 number = va_arg(args, UINT32);
                print_number(number, 10, false);
            }
            break;
            case u'd':
            {
                INT32 number = va_arg(args, INT32);
                print_number(number, 10, true);
            }
            break;
            case u'x':
            {
                UINTN hex = va_arg(args, UINTN);
                print_number(hex, 16, false);
            }
            break;
            case u'b':
            {
                UINTN hex = va_arg(args, UINTN);
                print_number(hex, 2, false);
            }
            break;
            case u'o':
            {
                UINTN hex = va_arg(args, UINTN);
                print_number(hex, 8, false);
            }
            break;
            default:
                cout->OutputString(cout, u"  Invalid format specifier: %");
                str[0] = fmt[i];
                cout->OutputString(cout, str);
                cout->OutputString(cout, u"\r\n");
                goto end;
                result = false;
                break;
            }
        }
        else
        {
            // Not formatted string, print next character
            str[0] = fmt[i];
            cout->OutputString(cout, str);
        }
    }
end:
    va_end(args);
    result = true;
    return result;
}