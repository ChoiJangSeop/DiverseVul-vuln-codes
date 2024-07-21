zzip_mem_disk_fdopen(int fd)
{
    ZZIP_DISK *disk = zzip_disk_mmap(fd);
    if (! disk)
    { 
       debug2("can not open disk fd %i", fd);
       return 0;
    }
    ___ ZZIP_MEM_DISK *dir = zzip_mem_disk_new();
    zzip_mem_disk_load(dir, disk);
    return dir;
    ____;
}