static void unzzip_mem_disk_cat_file(ZZIP_MEM_DISK* disk, char* name, FILE* out)
{
    ZZIP_DISK_FILE* file = zzip_mem_disk_fopen (disk, name);
    if (file) 
    {
	char buffer[1024]; int len;
	while ((len = zzip_mem_disk_fread (buffer, 1, 1024, file))) 
	{
	    fwrite (buffer, 1, len, out);
	}
	
	zzip_mem_disk_fclose (file);
    }
}