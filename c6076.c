zzip_mem_entry_fopen(ZZIP_MEM_DISK * dir, ZZIP_MEM_ENTRY * entry)
{
    /* keep this in sync with zzip_disk_entry_fopen */
    ZZIP_DISK_FILE *file = malloc(sizeof(ZZIP_MEM_DISK_FILE));
    if (! file)
        return file;
    file->buffer = dir->disk->buffer;
    file->endbuf = dir->disk->endbuf;
    file->avail = zzip_mem_entry_usize(entry);

    if (! file->avail || zzip_mem_entry_data_stored(entry))
        { file->stored = zzip_mem_entry_to_data (entry); return file; }

    file->stored = 0;
    file->zlib.opaque = 0;
    file->zlib.zalloc = Z_NULL;
    file->zlib.zfree = Z_NULL;
    file->zlib.avail_in = zzip_mem_entry_csize(entry);
    file->zlib.next_in = zzip_mem_entry_to_data(entry);

    if (! zzip_mem_entry_data_deflated(entry) ||
        inflateInit2(&file->zlib, -MAX_WBITS) != Z_OK)
        { free (file); return 0; }

    return file;
}