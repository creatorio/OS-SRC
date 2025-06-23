#include "uefilib.h"
EFI_EVENT timer_event;
EFI_STATUS efi_main(EFI_HANDLE ImageHandl, EFI_SYSTEM_TABLE *SystemTable)
{
    CHAR16 *EFI_ERROR_STRINGS[MAX_EFI_ERROR] = {
        [3] = u"EFI_UNSUPPORTED",
        [5] = u"EFI_BUFFER_TOO_SMALL",
        [7] = u"EFI_DEVICE_ERROR",
        [14] = u"EFI_NOT_FOUND",
        [27] = u"EFI_CRC_ERROR",
    };
    EFI_ERROR_STRINGS[1] = EFI_ERROR_STRINGS[1];
    INIT(SystemTable, ImageHandl);

    cin->Reset(cin, FALSE);
    cout->Reset(cout, FALSE);
    cerr->Reset(cerr, FALSE);

    cout->SetAttribute(cout, EFI_TEXT_ATTR(EFI_YELLOW, EFI_BLUE));

    bs->SetWatchdogTimer(0, 0x10000, 0, NULL);

    cout->ClearScreen(cout);
    /*
            printf(u"%x  ", u"\r");
            EFI_INPUT_KEY key = get_key();
            printf(u"%x", (CHAR16)key.UnicodeChar);
            get_key();*/
    bool running = true;

    printf(u"%d\r\n", (UINT32)42);
    get_key();
    const CHAR16 *Menu_Choices[11] = {
        u"Set Text Mode",
        u"Set Graphics Mode",
        u"Test Mouse",
        u"Read ESP File",
        u"Print Block IO Partitions",
        u"Print Memory Map",
        u"Print Configuration Tables",
        u"Print ACPI Tables",
        u"Print EFI Global Variables",
        u"Change Boot Variables",
        u"Load Kernel",
    };
    EFI_STATUS (*mfuncs[])(void) = {
        set_text_mode,
        set_graphics_mode,
        test_mouse,
        read_file_ESP,
        print_block_io_partitions,
        print_memory_map,
        print_configuration_table,
        print_ACPI_table,
        print_efi_global_variables,
        change_global_variables,
        load_kernel,
    };
    while (running)
    {
        cout->ClearScreen(cout);

        UINTN cols = 0, rows = 0;
        cout->QueryMode(cout, cout->Mode->Mode, &cols, &rows);
        text_cols = cols;
        text_rows = rows;
        screen_bounds datetime_context = {cols, rows};

        bs->CloseEvent(timer_event);

        bs->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, print_datetime, (void *)&datetime_context, &timer_event);

        bs->SetWatchdogTimer(0, 0x10000, 0, NULL);
        bs->SetTimer(timer_event, TimerPeriodic, 10000000);

        cout->SetCursorPosition(cout, 0, rows - 3);
        printf(u"  Up/Down Arrow = Move Cursor\r\n  Enter = Select\r\n  Escape = Shutdown");
        cout->SetCursorPosition(cout, 0, 0);
        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
        printf(u"  %s  ", Menu_Choices[0]);
        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
        for (UINTN i = 1; i < ArraySize(Menu_Choices); i++)
        {
            printf(u"\r\n  %s  ", Menu_Choices[i]);
        }

        INTN min_row = 0, max_row = cout->Mode->CursorRow;
        cout->SetCursorPosition(cout, 0, 0);
        bool getting_input = true;
        while (getting_input)
        {
            EFI_INPUT_KEY key = get_key();
            INTN currunt_row = cout->Mode->CursorRow;

            switch (key.ScanCode)
            {
            case UP_ARROW:

                cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                printf(u"                      \r  %s\r  ", Menu_Choices[currunt_row]);
                if (currunt_row - 1 >= min_row)
                {
                    currunt_row--;
                }
                else
                {
                    currunt_row = max_row;
                }
                cout->SetCursorPosition(cout, 0, currunt_row);
                cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                printf(u"                      \r  %s\r  ", Menu_Choices[currunt_row]);

                break;
            case DOWN_ARROW:

                cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                printf(u"                      \r  %s\r  ", Menu_Choices[currunt_row]);
                if (currunt_row + 1 <= max_row)
                {
                    currunt_row++;
                }
                else
                {
                    currunt_row = min_row;
                }
                cout->SetCursorPosition(cout, 0, currunt_row);
                cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                printf(u"                      \r  %s\r  ", Menu_Choices[currunt_row]);

                break;
            case ESC:
                bs->CloseEvent(timer_event);
                // rs->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
                return EFI_SUCCESS;
            default:
                if ((CHAR16)key.UnicodeChar == 0xd)
                {
                    mfuncs[currunt_row]();
                    getting_input = false;
                    break;
                }
            }
        }
    }

    return EFI_SUCCESS;
}
