zzip_disk_fread(void *ptr, zzip_size_t sized, zzip_size_t nmemb,
                ZZIP_DISK_FILE * file)
{
    zzip_size_t size = sized * nmemb;
    if (! ptr || ! sized || ! file)
        return 0;
    if (size > file->avail)
        size = file->avail;
    if (file->stored)
    {
        memcpy(ptr, file->stored, size);
        file->stored += size;
        file->avail -= size;
        return size;
    }

    file->zlib.avail_out = sized * nmemb;
    file->zlib.next_out = ptr;
    ___ zzip_size_t total_old = file->zlib.total_out;
    ___ int err = inflate(&file->zlib, Z_NO_FLUSH);
    if (err == Z_STREAM_END)
        file->avail = 0;
    else if (err == Z_OK)
        file->avail -= file->zlib.total_out - total_old;
    else
        return 0;
    return file->zlib.total_out - total_old;
    ____;
    ____;
}