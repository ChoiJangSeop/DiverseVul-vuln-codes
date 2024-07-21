zzip_mem_disk_fread (void* ptr, _zzip_size_t size, _zzip_size_t nmemb,
                     ZZIP_DISK_FILE* file) {
    return zzip_disk_fread(ptr, size, nmemb, file); }