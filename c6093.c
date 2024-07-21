zzip_mem_disk_fopen (ZZIP_MEM_DISK* dir, char* filename) {
    return zzip_disk_fopen(dir->disk, filename); }