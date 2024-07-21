zzip_mem_disk_findfile(ZZIP_MEM_DISK* dir, 
                       char* filename, ZZIP_DISK_ENTRY* after,
		       zzip_strcmp_fn_t compare) {
    return zzip_disk_findfile(dir->disk, filename, after, compare); }