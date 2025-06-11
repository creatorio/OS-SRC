#include "uefilib.h"
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *cout = NULL;
EFI_SIMPLE_TEXT_INPUT_PROTOCOL *cin = NULL;
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
Page_Table *pml4 = NULL;
EGWS CTGAS[6] = {
    {EFI_ACPI_TABLE_GUID, u"EFI_ACPI_TABLE_GUID"},
    {ACPI_TABLE_GUID, u"ACPI_TABLE_GUID"},
    {SAL_SYSTEM_TABLE_GUID, u"SAL_SYSTEM_TABLE_GUID"},
    {SMBIOS_TABLE_GUID, u"SMBIOS_TABLE_GUID"},
    {SMBIOS3_TABLE_GUID, u"SMBIOS3_TABLE_GUID"},
    {MPS_TABLE_GUID, u"MPS_TABLE_GUID"},
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

bool print_guid(EFI_GUID guid)
{
    UINT8 *p = (UINT8 *)&guid;
    return printf(u"\r\n{%x, %x, %x, %x, %x,{%x, %x, %x, %x, %x, %x}}\r\n",
                  *(UINT32 *)&p[0], *(UINT16 *)&p[4], *(UINT16 *)&p[6],
                  (UINTN)p[8], (UINTN)p[9], (UINTN)p[10], (UINTN)p[11],
                  (UINTN)p[12], (UINTN)p[13], (UINTN)p[14], (UINTN)p[15]);
}

VOID *read_esp_file_to_buffer(CHAR16 *path, UINTN *size)
{
    VOID *file_buffer = NULL;
    EFI_STATUS status;
    EFI_GUID lg = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *l;
    EFI_GUID sfspg = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfsp;
    EFI_FILE_PROTOCOL *root = NULL;
    EFI_FILE_PROTOCOL *file = NULL;
    EFI_FILE_INFO info;
    UINTN buf_size = sizeof(EFI_FILE_INFO);
    EFI_GUID fig = EFI_FILE_INFO_ID;

    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));

    status = bs->OpenProtocol(ih, &lg, (VOID **)&l, ih, NULL, ByProtocol);
    if (EFI_ERROR(status))
    {
        error(u"  ERROR: %x\r\nCould not locate Loaded Image Protocol handle buffer", status);
        goto cleanup;
    }
    status = bs->OpenProtocol(l->DeviceHandle, &sfspg, (VOID **)&sfsp, ih, NULL, ByProtocol);
    if (EFI_ERROR(status))
    {
        error(u"  ERROR: %x\r\nCould not locate Simple Filesystem Protocol handle buffer", status);
        goto cleanup;
    }
    status = sfsp->OpenVolume(sfsp, &root);

    if (EFI_ERROR(status))
    {
        error(u"  ERROR: %x\r\nCould not Open Volume for root directory in ESP", status);
        goto cleanup;
    }

    status = root->Open(root, &file, path, EFI_FILE_MODE_READ, 0);

    if (EFI_ERROR(status))
    {
        error(u"  ERROR: %x\r\nCould not Open File : %s", status, path);
        goto cleanup;
    }
    status = file->GetInfo(file, &fig, &buf_size, &info);
    if (EFI_ERROR(status))
    {
        error(u"  ERROR: %x\r\nCould not get file info for file: %s", status, path);
        goto cleanup;
    }

    buf_size = info.FileSize;
    status = bs->AllocatePool(EfiLoaderData, buf_size, &file_buffer);
    if (EFI_ERROR(status) || buf_size != info.FileSize)
    {
        error(u"  ERROR: %x\r\nCould not allocate memory pool for file: %s", status, path);
        goto cleanup;
    }

    status = file->Read(file, &buf_size, file_buffer);
    if (EFI_ERROR(status) || buf_size != info.FileSize)
    {
        error(u"  ERROR: %x\r\nCould not read for file %s into buffer", status, path);
        goto cleanup;
    }

    *size = buf_size;

cleanup:
    file->Close(file);
    root->Close(root);
    bs->CloseProtocol(ih, &lg, ih, NULL);
    bs->CloseProtocol(l->DeviceHandle, &sfspg, ih, NULL);
    return file_buffer;
}

EFI_STATUS get_disk_image_mediaID(UINT32 *ID)
{
    EFI_STATUS status = EFI_SUCCESS;

    EFI_GUID biopg = EFI_BLOCK_IO_PROTOCOL_GUID;
    EFI_GUID lipg = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_BLOCK_IO_PROTOCOL *biop;
    EFI_LOADED_IMAGE_PROTOCOL *lip;
    status = bs->OpenProtocol(ih, &lipg, (VOID **)&lip, ih, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status))
    {
        error(u"\r\nERROR: %x; Could Not Loaded Image Protocol. \r\n", status);
        return status;
    }
    status = bs->OpenProtocol(lip->DeviceHandle, &biopg, (VOID **)&biop, ih, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status))
    {
        error(u"\r\nERROR: %x; Could Not Open Block IO Protocol For This Disk. \r\n", status);
        return status;
    }
    *ID = biop->Media->MediaId;

    bs->CloseProtocol(lip->DeviceHandle, &biopg, ih, NULL);
    bs->CloseProtocol(ih, &lipg, ih, NULL);

    return EFI_SUCCESS;
}

EFI_PHYSICAL_ADDRESS read_disk_lbas_to_buffer(EFI_LBA disk_lba, UINTN data_size, UINT32 disk_mediaID, bool executable)
{
    EFI_PHYSICAL_ADDRESS buffer = 0;
    EFI_STATUS status = EFI_SUCCESS;
    EFI_GUID biopg = EFI_BLOCK_IO_PROTOCOL_GUID;
    EFI_BLOCK_IO_PROTOCOL *biop;
    UINTN num_handles = 0;
    EFI_HANDLE *handle_buffer = NULL;
    status = bs->LocateHandleBuffer(ByProtocol, &biopg, NULL, &num_handles, &handle_buffer);
    if (EFI_ERROR(status))
    {
        error(u"\r\nError: %x; Could Not Locate Any Block IO Protocols.\r\n", status);
        return buffer;
    };
    bool found = false;
    UINTN i = 0;
    for (; i < num_handles; i++)
    {
        status = bs->OpenProtocol(handle_buffer[i], &biopg, (VOID **)&biop, ih, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
        if (EFI_ERROR(status))
        {
            error(u"\r\nERROR: %x; Could Not Open Block IO Protocol On Handle %i. \r\n", status, i);
            bs->CloseProtocol(handle_buffer[i], &biopg, ih, NULL);
            continue;
        }
        if (biop->Media->MediaId == disk_mediaID && !biop->Media->LogicalPartition)
        {
            found = true;
            break;
        }
        bs->CloseProtocol(handle_buffer[i], &biopg, ih, NULL);
    }
    if (!found)
    {
        error(u"Could Not Find BLOCK IO protocol for entire disk for id %u", disk_mediaID);
        return buffer;
    }

    EFI_GUID diopg = EFI_DISK_IO_PROTOCOL_GUID;
    EFI_DISK_IO_PROTOCOL *diop;

    status = bs->OpenProtocol(handle_buffer[i], &diopg, (VOID **)&diop, ih, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status))
    {
        error(u"\r\nERROR: %x; Could Not Open DISK IO Protocol On Handle %i. \r\n", status, i);
        goto cleanup;
    }
    UINTN pages_needed = (data_size + (PAGE_SIZE - 1)) / PAGE_SIZE;
    // status = bs->AllocatePool(EfiLoaderData, max_memory_needed, &program_buffer);
    if (executable)
    {
        status = bs->AllocatePages(AllocateAnyPages, EfiLoaderCode, pages_needed, &buffer);
    }
    else
    {
        status = bs->AllocatePages(AllocateAnyPages, EfiLoaderData, pages_needed, &buffer);
    }
    // status = bs->AllocatePool(EfiLoaderData, data_size, &buffer);
    if (EFI_ERROR(status))
    {
        error(u"\r\nERROR: %x; Could Not allocat file buffer for disk data. \r\n", status);
        bs->CloseProtocol(handle_buffer[i], &diopg, ih, NULL);

        goto cleanup;
    }

    status = diop->ReadDisk(diop, disk_mediaID, (disk_lba * biop->Media->BlockSize), data_size, (VOID *)buffer);

    if (EFI_ERROR(status))
    {
        error(u"\r\nERROR: %x; Could Not read disk lbas into buffer. \r\n", status);
        bs->CloseProtocol(handle_buffer[i], &diopg, ih, NULL);
        goto cleanup;
    }
    bs->CloseProtocol(handle_buffer[i], &diopg, ih, NULL);

cleanup:
    bs->CloseProtocol(handle_buffer[i], &biopg, ih, NULL);
    return buffer;
}

VOID *memcpy(VOID *dst, VOID *src, UINTN len)
{
    UINTN *p = dst;
    UINTN *q = src;
    for (UINTN i = 0; i < len; i++)
    {
        p[i] = q[i];
    }
    return dst;
}
VOID *memset(VOID *dst, UINT8 c, UINTN len)
{
    UINT8 *p = dst;
    while (len--)
        *p++ = c;
    return dst;
}

INTN memcmp(VOID *m1, VOID *m2, UINTN len)
{
    if (len == 0)
        return 0;
    UINT8 *p = m1;
    UINT8 *q = m2;
    for (UINTN i = 0; i < len; i++)
    {
        if (p[i] != q[i])
            return (INTN)p[i] - (INTN)q[i];
    }
    return 0;
}
UINTN strlen(char *s)
{
    UINTN len = 0;
    while (*s)
    {
        len++;
        s++;
    }
    return len;
}
char *strstr(char *haystack, char *needle)
{
    if (!needle)
        return haystack;
    char *p = haystack;
    while (*p)
    {
        if (*p == *needle)
        {
            if (!memcmp(p, needle, strlen(needle)))
                return p;
        }
        p++;
    }
    return NULL;
}

bool isdigit(char c)
{
    return c >= '0' && c <= '9';
}

VOID *load_elf(VOID *elf_buffer, EFI_PHYSICAL_ADDRESS *file_buffer, UINTN *file_size)
{
    ELF_Header_64 *ehdr = elf_buffer;
    printf(u"Type: %u, Machine: %x, Entry: %x\r\n"
           "Pgm headers offset: %u, ELF Header size: %u\r\n"
           "Pm entry size: %u, # of pgm headers: %u\r\n",
           ehdr->e_type, ehdr->e_machine, ehdr->e_entry,
           ehdr->e_phoff, ehdr->e_ehsize,
           ehdr->e_phentsize, ehdr->e_phnum);
    if (ehdr->e_type != ET_DYN)
    {
        error(u"ELF is not PIE; e_type is not ETDYN/0x03\r\n");
        return NULL;
    }
    ELF_Program_Header_64 *phdr = (ELF_Program_Header_64 *)((UINT8 *)ehdr + ehdr->e_phoff);
    UINTN max_alignment = PAGE_SIZE;
    UINTN mem_min = UINT64_MAX, mem_max = 0;

    printf(u"Program Header:\r\n");
    for (UINT16 i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type != PT_LOAD)
        {
            printf(u"%u: Skipped, Not Loadable\r\n", (UINTN)i);
            continue;
        }
        printf(u"\r\n%u: Offset: %x, Vaddr: %x, Paddr: %x\r\n"
               u"    Filesize: %x, MemSize: %x, Alignment:%x\r\n",
               (UINTN)i, phdr[i].p_offset, phdr[i].p_vaddr, phdr[i].p_paddr,
               phdr[i].p_filesz, phdr[i].p_memsz, phdr[i].p_align);

        if (max_alignment < phdr[i].p_align)
            max_alignment = phdr[i].p_align;

        UINTN mem_begin = phdr[i].p_vaddr;
        UINTN mem_end = phdr[i].p_vaddr + phdr[i].p_memsz + max_alignment - 1;

        mem_begin &= ~(max_alignment - 1);
        mem_end &= ~(max_alignment - 1);

        if (mem_begin < mem_min)
            mem_min = mem_begin;
        if (mem_end > mem_max)
            mem_max = mem_end;
    }

    UINTN max_memory_needed = mem_max - mem_min;
    printf(u"Maximum Memory Needed For File: %x\r\n", max_memory_needed);

    EFI_STATUS status;
    EFI_PHYSICAL_ADDRESS program_buffer = 0;
    UINTN pages_needed = (max_memory_needed + (PAGE_SIZE - 1)) / PAGE_SIZE;
    // status = bs->AllocatePool(EfiLoaderData, max_memory_needed, &program_buffer);
    status = bs->AllocatePages(AllocateAnyPages, EfiLoaderCode, pages_needed, &program_buffer);
    if (EFI_ERROR(status))
    {
        error(u"  ERROR: %x\r\nCould not allocate memory for ELF program", status);
        return NULL;
    }

    memset((VOID *)program_buffer, 0, max_memory_needed);

    *file_buffer = program_buffer;
    *file_size = pages_needed * PAGE_SIZE;
    // printf(u"\r\n%x\r\n",ehdr->e_phnum);

    for (UINT16 i = 0; i < ehdr->e_phnum; i++)
    {

        if (phdr[i].p_type != PT_LOAD)
        {
            printf(u"%u: Skipped, Not Loadable\r\n", (UINTN)i);

            continue;
        }

        UINTN relative_offset = phdr[i].p_vaddr - mem_min;

        UINT8 *dst = (UINT8 *)program_buffer + relative_offset;
        UINT8 *src = (UINT8 *)elf_buffer + phdr[i].p_offset;
        UINT32 len = phdr[i].p_memsz;

        memcpy(dst, src, len);
    }

    VOID *entry_point = (VOID *)((UINT8 *)program_buffer + (ehdr->e_entry - mem_min));

    return entry_point;
}

VOID *load_pe(VOID *pe_buffer, EFI_PHYSICAL_ADDRESS *file_buffer, UINTN *file_size)
{
    UINT8 pe_sig_offset = 0x3C;
    UINT32 pe_sig_pos = *(UINT32 *)((UINT8 *)pe_buffer + pe_sig_offset);
    UINT8 *pe_sig = (UINT8 *)pe_buffer + pe_sig_pos;

    printf(u"\r\n\r\nPE Signature : [%x][%x][%x][%x]\r\n", (UINTN)pe_sig[0], (UINTN)pe_sig[1], (UINTN)pe_sig[2], (UINTN)pe_sig[3]);

    PE_Coff_File_Header_64 *coff_hdr = (PE_Coff_File_Header_64 *)(pe_sig + 4);

    printf(u"Machine: %x, # of sections: %u, Size of opt hdr: %x\r\n"
           u"Characteristics: %x\r\n",
           (UINTN)coff_hdr->Machine, (UINTN)coff_hdr->NumberOfSections, (UINTN)coff_hdr->SizeOfOptionalHeader,
           (UINTN)coff_hdr->Characteristics);

    if (coff_hdr->Machine != 0x8664)
    {
        error(u"Error: Machine type not AMD 64\r\n");
        return NULL;
    }
    if (!(coff_hdr->Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE))
    {
        error(u"Error: File not an executable image\r\n");
        return NULL;
    }

    PE_Optional_Header_64 *opt_hdr = (PE_Optional_Header_64 *)((UINT8 *)coff_hdr + sizeof(PE_Coff_File_Header_64));

    printf(u"Magic: %x, Entry Point: %x,\r\n"
           u"Sect Align: %x, File Align: %x, Size of Image: %x\r\n"
           u"Subsystem: %x, DLL Char: %x\r\n",
           (UINTN)opt_hdr->Magic, (UINTN)opt_hdr->AddressOfEntryPoint,
           (UINTN)opt_hdr->SectionAlignment, (UINTN)opt_hdr->FileAlignment, (UINTN)opt_hdr->SizeOfImage,
           (UINTN)opt_hdr->Subsystem, (UINTN)opt_hdr->DllCharacteristics);

    if (opt_hdr->Magic != 0x20B)
    {
        error(u"Error: File Is not PE32+\r\n");
        return NULL;
    }

    if (!(opt_hdr->DllCharacteristics & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE))
    {
        error(u"Error: File not PIE \r\n");
        return NULL;
    }

    EFI_PHYSICAL_ADDRESS program_buffer = 0;
    UINTN pages_needed = (opt_hdr->SizeOfImage + (PAGE_SIZE - 1)) / PAGE_SIZE;
    // EFI_STATUS status = bs->AllocatePool(EfiLoaderData, opt_hdr->SizeOfImage, &program_buffer);
    EFI_STATUS status = bs->AllocatePages(AllocateAnyPages, EfiLoaderCode, pages_needed, &program_buffer);
    if (EFI_ERROR(status))
    {
        error(u"Error %x : Could Not allocate memory for PE file\r\n", status);
        return NULL;
    }

    memset((VOID *)program_buffer, 0, opt_hdr->SizeOfImage);
    *file_buffer = program_buffer;
    *file_size = pages_needed * PAGE_SIZE;

    PE_Section_Header_64 *shdr = (PE_Section_Header_64 *)((UINT8 *)opt_hdr + coff_hdr->SizeOfOptionalHeader);

    printf(u"\r\n%x\r\n", shdr->SizeOfRawData);

    for (UINT16 i = 0; i < coff_hdr->NumberOfSections; i++, shdr++)
    {

        printf(u"Name: ");
        char *pos = (char *)&shdr->Name;
        for (UINT16 j = 0; j < 8; j++)
        {
            CHAR16 str[2];
            str[0] = *pos;
            str[1] = u'\0';
            if (*pos == '\0')
                break;
            printf(u"%s", str);
            pos++;
        }

        printf(u"VSize: %x, Vaddr: %x, Data Size: %x, Data Ptr%x\r\n",
               (UINTN)shdr->VirtualSize, (UINTN)shdr->VirtualAddress, (UINTN)shdr->SizeOfRawData, (UINTN)shdr->PointerToRawData);
    }

    shdr = (PE_Section_Header_64 *)((UINT8 *)opt_hdr + coff_hdr->SizeOfOptionalHeader);

    for (UINT16 i = 0; i < coff_hdr->NumberOfSections; i++, shdr++)
    {
        if (shdr->SizeOfRawData == 0)
            continue;

        VOID *dst = (UINT8 *)program_buffer + shdr->VirtualAddress;
        VOID *src = (UINT8 *)pe_buffer + shdr->PointerToRawData;
        UINTN len = shdr->SizeOfRawData;
        memcpy(dst, src, len);
    }

    VOID *entry_point = (UINT8 *)program_buffer + opt_hdr->AddressOfEntryPoint;

    return entry_point;
}

EFI_STATUS get_memory_map(Memory_Map_Info *mmap)
{
    EFI_STATUS status = EFI_SUCCESS;

    memset(mmap, 0, sizeof *mmap);

    status = bs->GetMemoryMap(&mmap->size, mmap->map, &mmap->key, &mmap->desc_size, &mmap->desc_version);
    if (EFI_ERROR(status) && status != EFI_BUFFER_TOO_SMALL)
    {
        error(u"Error %x: Could Not get Memory Map", status);
        return status;
    }

    mmap->size += mmap->desc_size * 2;
    status = bs->AllocatePool(EfiLoaderData, mmap->size, (VOID **)&mmap->map);
    if (EFI_ERROR(status))
    {
        error(u"Error %x: Could Not Allocate Memory for Memory Map", status);
        return status;
    }

    status = bs->GetMemoryMap(&mmap->size, mmap->map, &mmap->key, &mmap->desc_size, &mmap->desc_version);
    if (EFI_ERROR(status))
    {
        error(u"Error %x: Could Not get Memory Map", status);
        return status;
    }

    return status;
}

EFI_STATUS print_memory_map(void)
{
    EFI_STATUS status = EFI_SUCCESS;

    cout->ClearScreen(cout);

    bs->CloseEvent(timer_event);

    Memory_Map_Info mmap = {0};
    get_memory_map(&mmap);

    printf(u"Memory Map: Size: %x, Desc Size: %x, # of Descriptors: %u, Key: %x\r\n", mmap.size, mmap.desc_size, mmap.size / mmap.desc_size, mmap.key);

    UINTN usable_bytes = 0;
    for (UINTN i = 0; i < (mmap.size / mmap.desc_size); i++)
    {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mmap.map + i * mmap.desc_size);
        printf(u"%u: Typ: %u, Phy: %x, Vrt: %x, Pgs: %u, Att: %x\r\n", i, desc->Type, desc->PhysicalStart, desc->VirtualStart, desc->NumberOfPages, desc->Attribute);
        if (desc->Type == EfiLoaderCode ||
            desc->Type == EfiLoaderData ||
            desc->Type == EfiBootServicesCode ||
            desc->Type == EfiBootServicesData ||
            desc->Type == EfiConventionalMemory ||
            desc->Type == EfiPersistentMemory)
        {
            usable_bytes += desc->NumberOfPages * 4096;
        }
        if (i > 0 && i % 20 == 0)
        {
            get_key();
        }
    }

    error(u"\r\nUsable memory: %u / %u Mib / %u Gib", usable_bytes, usable_bytes / (1024 * 1024), usable_bytes / (1024 * 1024 * 1024));
    //    printf(u"after getkey");
    bs->FreePool(mmap.map);
    return status;
}

EFI_STATUS print_configuration_table(void)
{
    cout->ClearScreen(cout);
    EFI_STATUS status = EFI_SUCCESS;
    printf(u"Config:");

    for (UINTN i = 0; i < st->NumberOfTableEntries; i++)
    {
        EFI_GUID guid = st->ConfigurationTable[i].VendorGuid;
        print_guid(guid);
        UINTN j = 0;
        bool found = false;
        for (; j < ArraySize(CTGAS) && !found; j++)
        {
            if (!memcmp(&guid, &CTGAS[j].guid, sizeof guid))
            {
                found = true;
                break;
            }
        }

        printf(u"(%s)\r\n", (found ? CTGAS[j].string : u"Unknown GUID Value"));
        if (i != 0 && i % 7 == 0)
            get_key();
    }

    get_key();
    return status;
}

VOID *get_config_table_by_guid(EFI_GUID guid)
{
    for (UINTN i = 0; i < st->NumberOfTableEntries; i++)
    {
        EFI_GUID vendor_guid = st->ConfigurationTable[i].VendorGuid;

        if (!memcmp(&vendor_guid, &guid, sizeof guid))
            return st->ConfigurationTable[i].VendorTable;
    }

    return NULL;
}

void print_acpi_table_header(ACPI_Description_Header header)
{
    printf(u"XSDT:\r\n"
           u"Signature: %c%c%c%c\r\n"
           u"Length: %u\r\n"
           u"Revision: %u\r\n"
           u"Checksum: %u\r\n"
           u"QEMID: %c%c%c%c%c%c\r\n"
           u"QEM Table ID: %c%c%c%c%c%c%c%c\r\n"
           u"QEM Revision: %u\r\n"
           u"Creator ID: %x\r\n"
           u"Creator Revision: %x\r\n",
           header.signature[0], header.signature[1], header.signature[2], header.signature[3],
           (UINT32)header.length,
           (UINTN)header.revision,
           (UINTN)header.checksum,
           (UINTN)header.QEMID[0], (UINTN)header.QEMID[1], (UINTN)header.QEMID[2], (UINTN)header.QEMID[3], (UINTN)header.QEMID[4], (UINTN)header.QEMID[5],
           (UINTN)header.QEM_table_id[0], (UINTN)header.QEM_table_id[1], (UINTN)header.QEM_table_id[2], (UINTN)header.QEM_table_id[3], (UINTN)header.QEM_table_id[4], (UINTN)header.QEM_table_id[5], (UINTN)header.QEM_table_id[6], (UINTN)header.QEM_table_id[7],
           (UINTN)header.QEM_revision,
           (UINTN)header.creator_id,
           (UINTN)header.creator_revision);
}

EFI_STATUS print_ACPI_table(void)
{
    cout->ClearScreen(cout);
    EFI_STATUS status = EFI_SUCCESS;

    EFI_GUID acpig = EFI_ACPI_TABLE_GUID;
    bool acpi_20 = false;
    VOID *rsdp_ptr = get_config_table_by_guid(acpig);
    if (!rsdp_ptr)
    {
        acpig = (EFI_GUID)ACPI_TABLE_GUID;
        rsdp_ptr = get_config_table_by_guid(acpig);
        if (!rsdp_ptr)
        {
            error(u"Error: Could Not Fine ACPI Config Table");
            return 1;
        }
        else
        {
            printf(u"ACPI 1.0 Table found at: %x\r\n", rsdp_ptr);
        }
    }
    else
    {
        printf(u"ACPI 2.0 Table found at: %x\r\n", rsdp_ptr);
        acpi_20 = true;
    }

    UINT8 *rsdp = rsdp_ptr;
    if (acpi_20)
    {
        printf(u"RSDP:\r\n"
               u"Signature: %c%c%c%c%c%c%c%c\r\n"
               u"CheckSum: %u\r\n"
               u"QEMID: %c%c%c%c%c%c\r\n"
               u"RSDT Address: %x\r\n"
               u"Length: %u\r\n"
               u"XSDT Address: %x\r\n"
               u"Extended Checksum: %u\r\n",
               rsdp[0], rsdp[1], rsdp[2], rsdp[3], rsdp[4], rsdp[5], rsdp[6], rsdp[7],
               (UINTN)rsdp[8],
               rsdp[9], rsdp[10], rsdp[11], rsdp[12], rsdp[13], rsdp[14],
               *(UINT32 *)&rsdp[16],
               *(UINT32 *)&rsdp[20],
               *(UINT64 *)&rsdp[24],
               (UINTN)rsdp[32]);
    }
    else
    {
        printf(u"RSDP:\r\n"
               u"Signature: %c%c%c%c%c%c%c%c\r\n"
               u"CheckSum: %u\r\n"
               u"QEMID: %c%c%c%c%c%c\r\n"
               u"RSDT Address: %x\r\n",
               rsdp[0], rsdp[1], rsdp[2], rsdp[3], rsdp[4], rsdp[5], rsdp[6], rsdp[7],
               (UINTN)rsdp[8],
               rsdp[9], rsdp[10], rsdp[11], rsdp[12], rsdp[13], rsdp[14],
               *(UINT32 *)&rsdp[16]);
    }

    printf(u"Press any key to print RSDT/XSDT...");
    get_key();

    ACPI_Description_Header *header = NULL;
    UINT64 xsdt_addr = *(UINT64 *)&rsdp[24];
    if (acpi_20)
    {
        header = (ACPI_Description_Header *)xsdt_addr;
        print_acpi_table_header(*header);

        printf(u"Press any key to print entries...");

        get_key();

        printf(u"Entries:\r\n");
        UINT64 *entry = (UINT64 *)((UINT8 *)header + sizeof *header);
        for (UINTN i = 0; i < (header->length - sizeof *header) / 8; i++)
        {
            ACPI_Description_Header table_header = *(ACPI_Description_Header *)entry[i];
            printf(u"%c%c%c%c\r\n", table_header.signature[0], table_header.signature[1], table_header.signature[2], table_header.signature[3]);
        }
    }
    else
    {
    }

    get_key();
    return status;
}

void *mmap_allocate_pages(Memory_Map_Info *mmap, UINTN pages)
{
    static void *next_page_address = NULL;
    static UINTN current_descriptor = 0;
    static UINTN remaining_pages = 0;

    if (remaining_pages < pages)
    {
        UINTN i = current_descriptor + 1;
        for (; i < mmap->size / mmap->desc_size; i++)
        {
            EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mmap->map + (i * mmap->desc_size));
            if (desc->Type == EfiConventionalMemory && desc->NumberOfPages >= pages)
            {
                current_descriptor = i;
                remaining_pages = desc->NumberOfPages - pages;
                next_page_address = (void *)(desc->PhysicalStart + (pages * PAGE_SIZE));
                return (void *)desc->PhysicalStart;
            }
        }
        if (i >= mmap->size / mmap->desc_size)
        {
            error(u"ERROR: Could Not Find Any memory to allocate pages to");
            return NULL;
        }
    }

    remaining_pages -= pages;
    void *page = next_page_address;
    next_page_address = (void *)((UINT8 *)page + (pages * PAGE_SIZE));
    return page;
}

void map_page(UINTN physical_address, UINTN virtual_address, Memory_Map_Info *mmap)
{
    int flags = PRESENT | READWRITE | USER;

    // UINTN pml5_index = (virtual_address) >> 48 && 0x1FF;
    UINTN pml4_index = ((virtual_address) >> 39) && 0x1FF;
    UINTN pdpt_index = ((virtual_address) >> 30) && 0x1FF;
    UINTN pdt_index = ((virtual_address) >> 21) && 0x1FF;
    UINTN pt_index = ((virtual_address) >> 12) && 0x1FF;

    /*   if (!(pml5->entries[pml5_index] && PRESENT))
       {
           void *pml4_address = mmap_allocate_pages(mmap, 1);
           memset(pml4_address, 0, sizeof(Page_Table));
           pml5->entries[pml5_index] = (UINTN)pml4_address | flags;
       }
       Page_Table *pml4 = (Page_Table *)(pml5->entries[pml5_index] & PHYS_PAGE_ADDRESS_MASK);

          */

    if (!(pml4->entries[pml4_index] && PRESENT))
    {
        void *pdpt_address = mmap_allocate_pages(mmap, 1);
        memset(pdpt_address, 0, sizeof(Page_Table));
        pml4->entries[pml4_index] = (UINTN)pdpt_address | flags;
    }

    Page_Table *pdpt = (Page_Table *)(pml4->entries[pml4_index] & PHYS_PAGE_ADDRESS_MASK);
    if (!(pdpt->entries[pdpt_index] && PRESENT))
    {
        void *pdt_address = mmap_allocate_pages(mmap, 1);
        memset(pdt_address, 0, sizeof(Page_Table));
        pdpt->entries[pdpt_index] = (UINTN)pdt_address | flags;
    }

    Page_Table *pdt = (Page_Table *)(pdpt->entries[pdpt_index] & PHYS_PAGE_ADDRESS_MASK);
    if (!(pdt->entries[pdt_index] && PRESENT))
    {
        void *pt_address = mmap_allocate_pages(mmap, 1);
        memset(pt_address, 0, sizeof(Page_Table));
        pdt->entries[pdt_index] = (UINTN)pt_address | flags;
    }

    Page_Table *pt = (Page_Table *)(pdt->entries[pdt_index] & PHYS_PAGE_ADDRESS_MASK);
    if (!(pt->entries[pt_index] && PRESENT))
    {
        pt->entries[pt_index] = (physical_address & PHYS_PAGE_ADDRESS_MASK) | flags;
    }
}

void unmap_page(UINTN virtual_address)
{
    // UINTN pml5_index = (virtual_address) >> 48 && 0x1FF;
    UINTN pml4_index = ((virtual_address) >> 39) && 0x1FF;
    UINTN pdpt_index = ((virtual_address) >> 30) && 0x1FF;
    UINTN pdt_index = ((virtual_address) >> 21) && 0x1FF;
    UINTN pt_index = ((virtual_address) >> 12) && 0x1FF;

    //    Page_Table *pml4 = (Page_Table *)(pml5->entries[pml5_index] & PHYS_PAGE_ADDRESS_MASK);
    Page_Table *pdpt = (Page_Table *)(pml4->entries[pml4_index] & PHYS_PAGE_ADDRESS_MASK);
    Page_Table *pdt = (Page_Table *)(pdpt->entries[pdpt_index] & PHYS_PAGE_ADDRESS_MASK);
    Page_Table *pt = (Page_Table *)(pdt->entries[pdt_index] & PHYS_PAGE_ADDRESS_MASK);
    pt->entries[pt_index] = 0;

    __asm__ __volatile__("invlpg (%0)\n" : : "r"(virtual_address));
}

void identity_map_page(UINTN address, Memory_Map_Info *mmap)
{
    map_page(address, address, mmap);
}

void identity_map_efi_mmap(Memory_Map_Info *mmap)
{
    for (UINTN i = 0; i < mmap->size / mmap->desc_size; i++)
    {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mmap->map + (i * mmap->desc_size));
        for (UINTN j = 0; j < desc->NumberOfPages; j++)
        {
            identity_map_page(desc->PhysicalStart + (j * PAGE_SIZE), mmap);
        }
    }
}

void set_runtime_address_map(Memory_Map_Info *mmap)
{
    UINTN runtime_descriptors = 0;
    for (UINTN i = 0; i < mmap->size / mmap->desc_size; i++)
    {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mmap->map + (i * mmap->desc_size));
        if (desc->Attribute & EFI_MEMORY_RUNTIME)
            runtime_descriptors++;
    }
    UINTN runtime_mmap_pages = (runtime_descriptors * sizeof(EFI_MEMORY_DESCRIPTOR)) + ((PAGE_SIZE - 1) / PAGE_SIZE);
    EFI_MEMORY_DESCRIPTOR *runtime_mmap = mmap_allocate_pages(mmap, runtime_mmap_pages);
    if (!runtime_mmap)
    {
        error(u"ERROR: Could Not allocate runtime descriptos memory map");
        return;
    }

    UINTN runtime_mmap_size = runtime_mmap_pages * PAGE_SIZE;
    memset(runtime_mmap, 0, runtime_mmap_size);

    UINTN current_desc = 0;
    for (UINTN i = 0; i < mmap->size / mmap->desc_size; i++)
    {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mmap->map + (i * mmap->desc_size));
        if (desc->Attribute & EFI_MEMORY_RUNTIME)
        {
            EFI_MEMORY_DESCRIPTOR *runtime_desc = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)runtime_mmap + (current_desc * mmap->desc_size));
            memcpy(runtime_desc, desc, mmap->desc_size);
            runtime_desc->VirtualStart = runtime_desc->PhysicalStart;
            current_desc++;
        }
    }

    EFI_STATUS status = rs->SetVirtualAddressMap(runtime_mmap_size, mmap->desc_size, mmap->desc_version, runtime_mmap);
    if (EFI_ERROR(status))
    {
        error(u"Error: SetVirtualAddressMap()");
    }
}

EFI_STATUS load_kernel(void)
{
    VOID *disk_buffer = NULL;
    VOID *file_buffer = NULL;
    EFI_STATUS status = EFI_SUCCESS;

    CHAR16 *file_name = u"\\EFI\\BOOT\\FILE.TXT";
    UINTN file_size = 0, disk_lba = 0;

    cout->ClearScreen(cout);

    UINTN buf_size = 0;
    file_buffer = read_esp_file_to_buffer(file_name, &buf_size);
    if (!file_buffer)
    {
        error(u"Could Not Read File: %s", file_name);
        goto exit;
    }

    char *str_pos = NULL;
    str_pos = strstr(file_buffer, "kernel");
    if (!str_pos)
    {
        error(u"Could Not find kernel file in data partition\r\n");
        goto cleanup;
    }

    printf(u"Found kernel file\r\n");

    str_pos = strstr(file_buffer, "FILE_SIZE=");
    if (!str_pos)
    {
        error(u"Could Not find File size from buffer for: %s", file_name);
        goto cleanup;
    }

    str_pos += strlen("FILE_SIZE=");

    while (isdigit(*str_pos))
    {
        file_size = file_size * 10 + *str_pos - '0';
        str_pos++;
    };

    str_pos = strstr(file_buffer, "DISK_LBA=");
    if (!str_pos)
    {
        error(u"Could Not find disk lba from buffer for: %s", file_name);
        goto cleanup;
    }

    str_pos += strlen("DISK_LBA=");

    while (isdigit(*str_pos))
    {
        disk_lba = disk_lba * 10 + *str_pos - '0';
        str_pos++;
    };

    printf(u"File Size: %u, Disk LBA: %u\r\n", file_size, disk_lba);

    UINT32 mediaID = 0;
    status = get_disk_image_mediaID(&mediaID);
    if (EFI_ERROR(status))
    {
        error(u"Error: %x; Could Not Get MediaID value for disk image\r\n", status);
        bs->FreePool(file_buffer);
        goto exit;
    }

    disk_buffer = (VOID *)read_disk_lbas_to_buffer(disk_lba, file_size, mediaID, true);
    if (!disk_buffer)
    {
        error(u"Could Not Read Data partition file to buffer\r\n");
        bs->FreePool(file_buffer);
        goto exit;
    }

    Kernel_Parms kparms = {0};

    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

    status = bs->LocateProtocol(&gop_guid, NULL, (VOID **)&gop);
    if (EFI_ERROR(status))
    {
        error(u"  ERROR: %x\r\nCould not locate Graphics Output Protocol", status);
        return status;
    }

    kparms.gop_mode = *gop->Mode;
    Entry_Point entry_point = NULL;
    UINT8 *hdr = disk_buffer;
    printf(u"File Format: ");
    printf(u"Header Bytes: [%x][%x][%x][%x]", hdr[0], hdr[1], hdr[2], hdr[3]);

    EFI_PHYSICAL_ADDRESS kernel_buffer = 0;
    UINTN kernel_size = 0;

    if (!memcmp(hdr, (UINT8[4]){0x7F, 'E', 'L', 'F'}, 4))
    {
        *(void **)&entry_point = load_elf(disk_buffer, &kernel_buffer, &kernel_size);
    }
    else if (!memcmp(hdr, (UINT8[2]){'M', 'Z'}, 2))
    {
        *(void **)&entry_point = load_pe(disk_buffer, &kernel_buffer, &kernel_size);
    }
    else
    {
        printf(u"No Format Found assuming its a flat binary\r\n");
        *(void **)&entry_point = disk_buffer;
        kernel_buffer = (EFI_PHYSICAL_ADDRESS)disk_buffer;
        kernel_size = file_size;
    }

    printf(u"\r\n Kernel address: %x, size: %u, entry_point: %x\r\n", kernel_buffer, kernel_size, (UINTN)entry_point);

    if (!entry_point)
    {
        bs->FreePages(kernel_buffer, kernel_size / PAGE_SIZE);
        goto cleanup;
    }

    printf(u"press any key to load kernel");
    get_key();

    bs->CloseEvent(timer_event);

    if (EFI_ERROR(get_memory_map(&kparms.mmap)))
    {
        error(u"  ERROR: %x\r\nCould not Get Memory map", status);
        goto cleanup;
    }

    UINTN retries = 0;
    while (EFI_ERROR(bs->ExitBootServices(ih, kparms.mmap.key)) && retries < 10)
    {
        bs->FreePool(kparms.mmap.map);
        if (EFI_ERROR(get_memory_map(&kparms.mmap)))
        {
            goto cleanup;
        }
        goto cleanup;
        retries++;
    }

    if (retries == 10)
    {
        error(u"  ERROR: %x\r\nCould not Exit Boot Services", status);
        goto cleanup;
    }

    kparms.RuntimeServices = rs;
    kparms.NumberOfTableEntries = st->NumberOfTableEntries;
    kparms.ConfigurationTable = st->ConfigurationTable;

    pml4 = mmap_allocate_pages(&kparms.mmap, 1);
    memset(pml4, 0, sizeof *pml4);

    identity_map_efi_mmap(&kparms.mmap);

    set_runtime_address_map(&kparms.mmap);

    for (UINTN i = 0; i < (kernel_size + (PAGE_SIZE - 1)) / PAGE_SIZE; i++)
    {
        map_page(kernel_buffer + (PAGE_SIZE * i), KERNEL_START_ADDRESS + (PAGE_SIZE * i), &kparms.mmap);
    }


    UINTN entry_offset = (UINTN)entry_point - kernel_buffer;
    Entry_Point higher_entry_point = (Entry_Point)(KERNEL_START_ADDRESS + entry_offset);


    for (UINTN i = 0; i < (kparms.gop_mode.FrameBufferSize + (PAGE_SIZE - 1)) / PAGE_SIZE; i++)
    {
        identity_map_page(kparms.gop_mode.FrameBufferBase + (PAGE_SIZE * i), &kparms.mmap);
    }

    void *kernel_stack = mmap_allocate_pages(&kparms.mmap, STACK_PAGES);
    memset(kernel_stack, 0, STACK_PAGES * PAGE_SIZE);

    for (UINTN i = 0; i < STACK_PAGES; i++)
    {
        identity_map_page((UINTN)kernel_stack + (i * PAGE_SIZE), &kparms.mmap);
    }

    TSS tss = {.io_map_base = sizeof(TSS)};
    UINTN tss_address = (UINTN)&tss;

    GDT gdt = {
        .null.value = 0x0000000000000000, // Null descriptor
        .kernel_code_64.value = 0x00AF9A000000FFFF,
        .kernel_data_64.value = 0x00CF92000000FFFF,
        .user_code_64.value = 0x00AFFA000000FFFF,
        .user_data_64.value = 0x00CFF2000000FFFF,
        .kernel_code_32.value = 0x00CF9A000000FFFF,
        .kernel_data_32.value = 0x00CF92000000FFFF,
        .user_code_32.value = 0x00CFFA000000FFFF,
        .user_data_32.value = 0x00CFF2000000FFFF,
        .tss = {
            .descriptor = {
                .limit_15_0 = sizeof tss - 1,
                .base_15_0 = tss_address & 0xFFFF,
                .base_23_16 = (tss_address >> 16) & 0xFF,
                .type = 9, // 0b1001 64 bit TSS (available)
                .p = 1,    // Present
                .base_31_24 = (tss_address >> 24) & 0xFF,
            },
            .base_63_32 = (tss_address >> 32) & 0xFFFFFFFF,
        }};

    Descriptor_Register gdtr = {.limit = sizeof gdt - 1 , .base = (UINT64)&gdt};

    Kernel_Parms *kparms_ptr = &kparms;
    UINT32 *fb = (UINT32 *)kparms.gop_mode.FrameBufferBase;
    UINT32 xres = kparms.gop_mode.Info->HorizontalResolution;
    UINT32 yres = kparms.gop_mode.Info->VerticalResolution;

    for (UINT32 y = 0; y < yres; y++)
    {
        for (UINT32 x = 0; x < xres; x++)
        {
            fb[y * xres + x] = 0xFFDDDDDD;
        }
    }
    //entry_point(kparms_ptr);
    __asm__ __volatile__(
        "cli\n"                     // Clear interrupts before setting new GDT/TSS, etc.
        "movq %[pml4], %%rax\n"     // Load new page tables
        "movq %%rax, %%CR3\n"     // Load new page tables
        "lgdt %[gdt]\n"             // Load new GDT from gdtr register
        "ltr %[tss]\n"              // Load new task register with new TSS value (byte offset into GDT)

        // Jump to new code segment in GDT (offset in GDT of 64 bit kernel/system code segment)
        "pushq $0x8\n"
        "leaq 1f(%%RIP), %%RAX\n"
        "pushq %%RAX\n"
        "lretq\n"

        // Executing code with new Code segment now, set up remaining segment registers
        "1:\n"
        "movq $0x10, %%RAX\n"   // Data segment to use (64 bit kernel data segment, offset in GDT)
        "movq %%RAX, %%DS\n"    // Data segment
        "movq %%RAX, %%ES\n"    // Extra segment
        "movq %%RAX, %%FS\n"    // Extra segment (2), these also have different uses in Long Mode
        "movq %%RAX, %%GS\n"    // Extra segment (3), these also have different uses in Long Mode
        "movq %%RAX, %%SS\n"    // Stack segment

        // Set new stack value to use (for SP/stack pointer, etc.)
        "movq %[stack], %%RSP\n"

        // Call new entry point in higher memory
        "callq *%[entry]\n" // First parameter is kparms in RCX in input constraints below, for MS ABI
      :
      : [pml4]"r"(pml4), [gdt]"m"(gdtr), [tss]"r"((uint16_t)offsetof(GDT, tss)),
        [stack]"gm"((uint64_t)kernel_stack + (STACK_PAGES * PAGE_SIZE)),    // Top of newly allocated stack
        [entry]"r"(higher_entry_point), "c"(kparms_ptr)
      : "rax", "memory");

    __builtin_unreachable();

cleanup:
    bs->FreePool(file_buffer);
    bs->FreePool(disk_buffer);

exit:
    printf(u"\r\nPress any key to go back\r\n");
    get_key();
    return status;
}

EFI_STATUS print_block_io_partitions(void)
{
    EFI_STATUS status = EFI_SUCCESS;

    cout->ClearScreen(cout);
    EFI_GUID biopg = EFI_BLOCK_IO_PROTOCOL_GUID;
    EFI_GUID lipg = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_BLOCK_IO_PROTOCOL *biop;
    EFI_LOADED_IMAGE_PROTOCOL *lip;
    UINTN num_handles = 0;
    EFI_HANDLE *handle_buffer = NULL;
    UINT32 lmi = -1;
    EFI_GUID Basic_Data_GUID = Basic_Data_GUID;
    EFI_GUID pipg = EFI_PARTITION_INFO_PROTOCOL_GUID;
    EFI_PARTITION_INFO_PROTOCOL *pip;
    status = bs->OpenProtocol(ih, &lipg, (VOID **)&lip, ih, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status))
    {
        error(u"\r\nERROR: %x; Could Not Loaded Image Protocol. \r\n", status);
        return status;
    }
    status = bs->OpenProtocol(lip->DeviceHandle, &biopg, (VOID **)&biop, ih, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status))
    {
        error(u"\r\nERROR: %x; Could Not Open Block IO Protocol For This Disk. \r\n", status);
        return status;
    }
    UINT32 timi = biop->Media->MediaId;

    bs->CloseProtocol(lip->DeviceHandle, &biopg, ih, NULL);
    bs->CloseProtocol(ih, &lipg, ih, NULL);

    status = bs->LocateHandleBuffer(ByProtocol, &biopg, NULL, &num_handles, &handle_buffer);
    if (EFI_ERROR(status))
    {
        error(u"\r\nError: %x; Could Not Locate Any Block IO Protocols.\r\n", status);
        return status;
    };

    for (UINTN i = 0; i < num_handles; i++)
    {
        status = bs->OpenProtocol(handle_buffer[i], &biopg, (VOID **)&biop, ih, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
        if (EFI_ERROR(status))
        {
            error(u"\r\nERROR: %x; Could Not Open Block IO Protocol On Handle %i. \r\n", status, i);
            continue;
        }
        if (lmi != biop->Media->MediaId)
        {
            lmi = biop->Media->MediaId;
            printf(u"Media ID: %u %s \r\n\r\n", lmi, (lmi == timi ? u"(Disk Image)" : u""));
        }

        if (biop->Media->LastBlock == 0)
        {
            bs->CloseProtocol(handle_buffer[i], &biopg, ih, NULL);
            continue;
        }

        if (!biop->Media->LogicalPartition)
        {
            printf(u"<Entire Disk>\r\n");
        }
        else
        {
            status = bs->OpenProtocol(handle_buffer[i], &pipg, (VOID **)&pip, ih, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
            if (EFI_ERROR(status))
            {
                error(u"\r\nERROR: %x; Could Not Open Partition Info Protocol On Handle %i. \r\n", status, i);
                continue;
            }
            else
            {
                if (pip->Type == PARTITION_TYPE_OTHER)
                    printf(u"<OTHER>\r\n");
                else if (pip->Type == PARTITION_TYPE_GPT)
                {
                    if (pip->System)
                        printf(u"<ESP>\r\n");
                    else
                    {
                        if (!memcmp(&pip->Info.Gpt.PartitionTypeGUID, &Basic_Data_GUID, sizeof(EFI_GUID)))
                        {
                            printf(u"<Basic Data>\r\n");
                        }
                        else
                        {
                            printf(u"<Other GPT Type>\r\n");
                        }
                    };
                }
                else if (pip->Type == PARTITION_TYPE_MBR)
                    printf(u"<MBR>");
            }
        }

        printf(u"Rmv: %s; Pr: %s; ISPT: %s; RdOnly: %s; WrtCh: %s;\r\nBlkSz: %u; IOA: %u; LstBlk: %u; LwLBA: %u; LgBlk/Phy: %u;\r\nOptTrnLenGrn: %u;\r\n",
               biop->Media->RemovableMedia ? u"Yes" : u"No",
               biop->Media->MediaPresent ? u"Yes" : u"No",
               biop->Media->LogicalPartition ? u"Yes" : u"No",
               biop->Media->ReadOnly ? u"Yes" : u"No",
               biop->Media->WriteCaching ? u"Yes" : u"No",
               biop->Media->BlockSize,
               biop->Media->IoAlign,
               biop->Media->LastBlock,
               biop->Media->LowestAlignedLba,
               biop->Media->LogicalBlocksPerPhysicalBlock,
               biop->Media->OptimalTransferLengthGranularity);

        bs->CloseProtocol(handle_buffer[i], &biopg, ih, NULL);
        printf(u"\r\n");
    }

    printf(u"Press any key to return\r\n");
    get_key();
    return status;
}

CHAR16 *strcpy_u16(CHAR16 *dst, const CHAR16 *src)
{
    if (!dst)
        return NULL;
    if (!src)
        return dst;

    CHAR16 *result = dst;
    while (*src)
        *dst++ = *src++;

    *dst = u'\0';

    return result;
}

INTN strncmp_u16(CHAR16 *s1, CHAR16 *s2, UINTN n)
{
    if (n == 0)
        return 0;
    while (n > 0 && *s1 && *s2 && *s1 == *s2)
    {
        s1++;
        s2++;
        n--;
    }
    return *s1 - *s2;
}

CHAR16 *strrchar_u16(CHAR16 *str, CHAR16 c)
{
    CHAR16 *result = NULL;
    while (*str)
    {
        if (*str == c)
            result = str;
        str++;
    }

    return result;
}

CHAR16 *strcat_u16(CHAR16 *str1, CHAR16 *str2)
{
    CHAR16 *result = str1;
    while (*result)
        result++;
    while (*str2)
        *result++ = *str2++;
    *result = u'\0';
    return str1;
}

EFI_STATUS read_file_ESP(void)
{
    EFI_GUID lg = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *l;
    EFI_GUID sfspg = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfsp;
    EFI_STATUS status = EFI_SUCCESS;

    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));

    status = bs->OpenProtocol(ih, &lg, (VOID **)&l, ih, NULL, ByProtocol);
    if (EFI_ERROR(status))
    {
        error(u"  ERROR: %x\r\nCould not locate Loaded Image Protocol handle buffer", status);
        goto cleanup;
    }
    status = bs->OpenProtocol(l->DeviceHandle, &sfspg, (VOID **)&sfsp, ih, NULL, ByProtocol);
    if (EFI_ERROR(status))
    {
        error(u"  ERROR: %x\r\nCould not locate Simple Filesystem Protocol handle buffer", status);
        goto cleanup;
    }
    EFI_FILE_PROTOCOL *dirp = NULL;
    status = sfsp->OpenVolume(sfsp, &dirp);

    if (EFI_ERROR(status))
    {
        error(u"  ERROR: %x\r\nCould not Open Root", status);
        goto cleanup;
    }
    CHAR16 current_directory[256];
    strcpy_u16(current_directory, u"/");
    INT32 csr_row = 1;
    while (true)
    {
        cout->ClearScreen(cout);
        printf(u"%s:\r\n", current_directory);

        INT32 num_entries = 0;

        EFI_FILE_INFO file_info;
        UINTN buf_size = sizeof(file_info);
        dirp->SetPosition(dirp, 0);
        dirp->Read(dirp, &buf_size, &file_info);
        while (buf_size > 0)
        {
            num_entries++;
            if (csr_row == cout->Mode->CursorRow)
            {
                cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
            }
            printf(u"%s %s\r\n", (file_info.Attribute & EFI_FILE_DIRECTORY) ? u"[DIR] " : u"[FILE]",
                   file_info.FileName);
            if (csr_row + 1 == cout->Mode->CursorRow)
            {
                cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
            }
            buf_size = sizeof file_info;
            dirp->Read(dirp, &buf_size, &file_info);
        }
        EFI_INPUT_KEY key = get_key();
        switch (key.ScanCode)
        {
        case ESC:
            dirp->Close(dirp);
            goto cleanup;
            break;
        case UP_ARROW:
            if (csr_row > 1)
                csr_row--;
            break;
        case DOWN_ARROW:
            if (csr_row < num_entries)
                csr_row++;
            break;
        default:
            if ((CHAR16)key.UnicodeChar == 0xd)
            {

                dirp->SetPosition(dirp, 0);
                INT32 i = 0;
                do
                {
                    buf_size = sizeof file_info;
                    dirp->Read(dirp, &buf_size, &file_info);
                    i++;
                } while (i < csr_row);

                if (file_info.Attribute & EFI_FILE_DIRECTORY)
                {
                    EFI_FILE_PROTOCOL *new_dir;
                    status = dirp->Open(dirp, &new_dir, file_info.FileName, EFI_FILE_MODE_READ, 0);

                    if (EFI_ERROR(status))
                    {
                        printf(u"  ERROR: %x\r\nCould not Open Directory or File", status);
                        goto cleanup;
                    }
                    dirp->Close(dirp);
                    dirp = new_dir;
                    csr_row = 1;

                    if (!strncmp_u16(file_info.FileName, u".", 2))
                    {
                    }
                    else if (!strncmp_u16(file_info.FileName, u"..", 3))
                    {
                        CHAR16 *pos = strrchar_u16(current_directory, u'/');
                        if (pos == current_directory)
                            pos++;
                        *pos = u'\0';
                    }
                    else
                    {
                        if (current_directory[1] != u'\0')
                        {
                            strcat_u16(current_directory, u"/");
                        }

                        strcat_u16(current_directory, file_info.FileName);
                    }
                    continue;
                }

                VOID *buffer = NULL;
                buf_size = file_info.FileSize;
                status = bs->AllocatePool(EfiLoaderData, buf_size, &buffer);
                if (EFI_ERROR(status))
                {
                    printf(u"  ERROR: %x\r\nCould not allocate memory for file: %s", status, file_info.FileName);
                    get_key();
                    goto cleanup;
                }
                EFI_FILE_PROTOCOL *file;
                status = dirp->Open(dirp, &file, file_info.FileName, EFI_FILE_MODE_READ, 0);
                if (EFI_ERROR(status))
                {
                    char *pos = (char *)buffer;
                    for (UINTN bytes = buf_size; bytes > 0; bytes--)
                    {
                        CHAR16 str[2];
                        str[0] = *pos;
                        str[1] = u'\0';
                        if (*pos == '\n')
                        {
                            printf(u"\r\n");
                        }
                        else
                        {
                            printf(u"%s", str);
                        }
                        pos++;
                    }
                    printf(u"  ERROR: %x\r\nCould not Open Files: %s", status, file_info.FileName);
                    goto cleanup;
                }
                status = dirp->Read(file, &buf_size, buffer);
                if (EFI_ERROR(status))
                {
                    printf(u"  ERROR: %x\r\nCould not read file: %s in buffer", status, file_info.FileName);
                    get_key();
                    goto cleanup;
                }
                if (buf_size != file_info.FileSize)
                {
                    printf(u"  ERROR: Could not read all of file: %s into buffer.\r\n"
                           u"Bytes Read: %u, Expected: %u\r\n",
                           file_info.FileName, buf_size, file_info.FileSize);
                    get_key();
                    goto cleanup;
                }

                char *pos = (char *)buffer;
                for (UINTN bytes = buf_size; bytes > 0; bytes--)
                {
                    CHAR16 str[2];
                    str[0] = *pos;
                    str[1] = u'\0';
                    if (*pos == '\n')
                    {
                        printf(u"\r\n");
                    }
                    else
                    {
                        printf(u"%s", str);
                    }
                    pos++;
                }
                get_key();
                bs->FreePool(buffer);
                dirp->Close(file);
            }
            break;
        }
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
    UINTN num_handles = 0;
    EFI_HANDLE *handle_buffer = NULL;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *mode_info = NULL;
    UINTN mode_size = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
    INTN cursor_size = 8;
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
        };
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
        error(u"  ERROR: %x\r\nCould not locate Simple Pointer Protocol handle buffer", status);
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
        error(u"  ERROR: %x\r\nCould not locate Absolute Pointer Protocol handle buffer", status);
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
    }

    printf(u"\r\n  Mouse Xmm: %d, Ymm: %d, LB: %d, RB: %d\r", 0, 0, 0, 0);
    INT32 xr = mode_info->HorizontalResolution, yr = mode_info->VerticalResolution;
    cursorx = (xr / 2) - (cursor_size / 2);
    cursory = (yr / 2) - (cursor_size / 2);
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *fb = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)gop->Mode->FrameBufferBase;
    for (INTN x = 0; x < cursor_size; x++)
    {
        for (INTN y = 0; y < cursor_size; y++)
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
                return EFI_SUCCESS;
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
            fb = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)gop->Mode->FrameBufferBase;

            for (INTN x = 0; x < cursor_size; x++)
            {
                for (INTN y = 0; y < cursor_size; y++)
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
            for (INTN x = 0; x < cursor_size; x++)
            {
                for (INTN y = 0; y < cursor_size; y++)
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

            for (INTN x = 0; x < cursor_size; x++)
            {
                for (INTN y = 0; y < cursor_size; y++)
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
            for (INTN x = 0; x < cursor_size; x++)
            {
                for (INTN y = 0; y < cursor_size; y++)
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
        error(u"  ERROR: %x\r\nCould not locate Graphics Output Protocol", status);
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
        error(u"  ERROR: %x\r\nCould not query Graphics Output Protocol Mode %x", status, gop->Mode->Mode);
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

    const UINT32 max = gop->Mode->MaxMode;
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
        EFI_INPUT_KEY key = get_key();
        /*
                CHAR16 k[2];
                k[0] = key.UnicodeChar;
                k[1] = (uintptr_t)u"\0";
                // printf(u"Scancode: %x\r\nUnicodeChar: %s\r\n", key.ScanCode, k);
                UINTN mode = key.UnicodeChar - u'0';
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
            getting_input = false;
            break;

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
    return EFI_SUCCESS;
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

        // CHAR16 k[2];
        EFI_INPUT_KEY key = get_key();
        // k[0] = key.UnicodeChar;
        // k[1] = (uintptr_t)u"\0";
        //  printf(u"Scancode: %x\r\nUnicodeChar: %s\r\n", key.ScanCode, k);
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
            getting_input = false;
            break;

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
    return EFI_SUCCESS;
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
    CHAR16 buffer[32]; // Hopefully enough for UINTN_MAX (UINT64_MAX) + sign character
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
    bool result = true;
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
    return result;
}
bool error(CHAR16 *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool result = printf(fmt, args);
    va_end(args);
    get_key();
    return result;
}
