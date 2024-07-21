zzip_mem_disk_entry_free(ZZIP_MEM_DISK_ENTRY* _zzip_restrict item) 
{
    if (item) {
	if (item->zz_extras) free(item->zz_extras);
	free (item);
    }
}