zzip_mem_disk_buffer(char *buffer, size_t buflen)
{
    ZZIP_DISK *disk = zzip_disk_buffer(buffer, buflen);
    if (! disk)
    { 
       debug2("can not open disk buf %p", buffer);
       return 0;
    }
    ___ ZZIP_MEM_DISK *dir = zzip_mem_disk_new();
    zzip_mem_disk_load(dir, disk);
    return dir;
    ____;
}