zzip_mem_disk_entry_to_file_header(ZZIP_MEM_DISK* dir, 
				   ZZIP_DISK_ENTRY* entry) {
    return zzip_disk_entry_to_file_header(dir->disk, entry); }