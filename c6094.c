zzip_mem_disk_findmatch(ZZIP_MEM_DISK* dir, 
                        char* filespec, ZZIP_DISK_ENTRY* after,
			zzip_fnmatch_fn_t compare, int flags) {
    return zzip_disk_findmatch(dir->disk, filespec, after, compare, flags); }