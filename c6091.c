zzip_mem_disk_open(char* filename)
{
    ZZIP_DISK* disk = zzip_disk_open(filename);
    if (! disk) { perror(error[_zzip_mem_disk_open_fail]); return 0; }
    ___ ZZIP_MEM_DISK* dir = calloc(1, sizeof(*dir)); 
    zzip_mem_disk_load(dir, disk);
    return dir; ____;
}