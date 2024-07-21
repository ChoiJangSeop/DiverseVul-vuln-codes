zzip_mem_disk_entry_fopen (ZZIP_MEM_DISK* dir, ZZIP_DISK_ENTRY* entry) {
    return zzip_disk_entry_fopen(dir->disk, entry); }