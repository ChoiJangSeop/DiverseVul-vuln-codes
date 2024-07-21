zzip_mem_disk_load(ZZIP_MEM_DISK* dir, ZZIP_DISK* disk)
{
    if (dir->list) zzip_mem_disk_unload(dir);
    ___ struct zzip_disk_entry* entry = zzip_disk_findfirst(disk);
    for (; entry ; entry = zzip_disk_findnext(disk, entry)) {
	ZZIP_MEM_DISK_ENTRY* item = zzip_mem_disk_entry_new(disk, entry);
	if (dir->last) { dir->last->zz_next = item; }
	else { dir->list = item; }; dir->last = item;
    } ____;
    dir->disk = disk;
    return 0;
}