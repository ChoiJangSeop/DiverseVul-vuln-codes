read_library (WinLibrary *fi)
{
    /* check for DOS header signature `MZ' */
    RETURN_IF_BAD_POINTER(false, MZ_HEADER(fi->memory)->magic);
    if (MZ_HEADER(fi->memory)->magic == IMAGE_DOS_SIGNATURE) {
        DOSImageHeader *mz_header = MZ_HEADER(fi->memory);

        RETURN_IF_BAD_POINTER(false, mz_header->lfanew);
        if (mz_header->lfanew < sizeof (DOSImageHeader)) {
            warn(_("%s: not a PE or NE library"), fi->name);
            return false;
        }

        /* falls through */
    }

    RETURN_IF_BAD_OFFSET(false, MZ_HEADER(fi->memory), sizeof(Win32ImageNTHeaders));
    /* check for OS2 (Win16) header signature `NE' */
    RETURN_IF_BAD_POINTER(false, NE_HEADER(fi->memory)->magic);
    if (NE_HEADER(fi->memory)->magic == IMAGE_OS2_SIGNATURE) {
        OS2ImageHeader *header = NE_HEADER(fi->memory);
        uint16_t *alignshift;

        RETURN_IF_BAD_POINTER(false, header->rsrctab);
        RETURN_IF_BAD_POINTER(false, header->restab);
        if (header->rsrctab >= header->restab) {
            warn(_("%s: no resource directory found"), fi->name);
            return false;
        }

        fi->is_PE_binary = false;
        alignshift = (uint16_t *) ((uint8_t *) NE_HEADER(fi->memory) + header->rsrctab);
        fi->first_resource = ((uint8_t *) alignshift) + sizeof(uint16_t);
        RETURN_IF_BAD_POINTER(false, *(Win16NETypeInfo *) fi->first_resource);

        return true;
    }

    /* check for NT header signature `PE' */
    RETURN_IF_BAD_POINTER(false, PE_HEADER(fi->memory)->signature);
    if (PE_HEADER(fi->memory)->signature == IMAGE_NT_SIGNATURE) {
        Win32ImageSectionHeader *pe_sec;
        Win32ImageDataDirectory *dir;
        Win32ImageNTHeaders *pe_header;
        int d;

        /* allocate new memory */
        fi->total_size = calc_vma_size(fi);
        if (fi->total_size <= 0) {
            /* calc_vma_size has reported error */
            return false;
        }
        fi->memory = xrealloc(fi->memory, fi->total_size);

        /* relocate memory, start from last section */
        pe_header = PE_HEADER(fi->memory);
        RETURN_IF_BAD_POINTER(false, pe_header->file_header.number_of_sections);
        RETURN_IF_BAD_PE_SECTIONS(false, fi->memory);

        /* we don't need to do OFFSET checking for the sections.
         * calc_vma_size has already done that */
        for (d = pe_header->file_header.number_of_sections - 1; d >= 0 ; d--) {
            pe_sec = PE_SECTIONS(fi->memory) + d;

            if (pe_sec->characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
                continue;

            //if (pe_sec->virtual_address + pe_sec->size_of_raw_data > fi->total_size)

            RETURN_IF_BAD_OFFSET(0, fi->memory + pe_sec->virtual_address, pe_sec->size_of_raw_data);
            RETURN_IF_BAD_OFFSET(0, fi->memory + pe_sec->pointer_to_raw_data, pe_sec->size_of_raw_data);
            if (pe_sec->virtual_address != pe_sec->pointer_to_raw_data) {
                memmove(fi->memory + pe_sec->virtual_address,
                fi->memory + pe_sec->pointer_to_raw_data,
                pe_sec->size_of_raw_data);
            }
        }

        /* find resource directory */
        dir = get_data_directory_entry (fi, IMAGE_DIRECTORY_ENTRY_RESOURCE);
        if (dir == NULL) return false;
        if (dir->size == 0) {
            warn(_("%s: file contains no resources"), fi->name);
            return false;
        }

        fi->first_resource = ((uint8_t *) fi->memory) + dir->virtual_address;
        fi->is_PE_binary = true;
        return true;
    }

    /* other (unknown) header signature was found */
    warn(_("%s: not a PE or NE library"), fi->name);
    return false;
}