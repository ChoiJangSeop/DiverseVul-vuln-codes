static int unzzip_cat (int argc, char ** argv, int extract)
{
    int done;
    int argn;
    ZZIP_MEM_DISK* disk;

    if (argc == 1)
    {
	printf (__FILE__" version "ZZIP_PACKAGE" "ZZIP_VERSION"\n");
	return EXIT_OK; /* better provide an archive argument */
    }

    disk = zzip_mem_disk_open (argv[1]);
    if (! disk) {
        DBG3("disk_open failed [%i] %s", errno, strerror(errno));
	perror(argv[1]);
	return exitcode(errno);
    }

    if (argc == 2)
    {  /* print directory list */
	ZZIP_MEM_ENTRY* entry = zzip_mem_disk_findfirst(disk);
	DBG2("findfirst %p\n", entry);
	for (; entry ; entry = zzip_mem_disk_findnext(disk, entry))
	{
	    char* name = zzip_mem_entry_to_name (entry);
	    FILE* out = stdout;
	    if (extract) out = create_fopen(name, "wb", 1);
	    if (! out) {
	        if (errno != EISDIR) done = EXIT_ERRORS;
	        continue;
	    }
	    unzzip_mem_disk_cat_file (disk, name, out);
	    if (extract) fclose(out);
	}
	return done;
    }

    if (argc == 3 && !extract)
    {  /* list from one spec */
	ZZIP_MEM_ENTRY* entry = 0;
	while ((entry = zzip_mem_disk_findmatch(disk, argv[2], entry, 0, 0)))
	{
	     unzzip_mem_entry_fprint (disk, entry, stdout);
	}

	return 0;
    }

    for (argn=1; argn < argc; argn++)
    {   /* list only the matching entries - each in order of commandline */
	ZZIP_MEM_ENTRY* entry = zzip_mem_disk_findfirst(disk);
	for (; entry ; entry = zzip_mem_disk_findnext(disk, entry))
	{
	    char* name = zzip_mem_entry_to_name (entry);
	    if (! _zzip_fnmatch (argv[argn], name, 
		_zzip_FNM_NOESCAPE|_zzip_FNM_PATHNAME|_zzip_FNM_PERIOD))
	    {
	        FILE* out = stdout;
	        if (extract) out = create_fopen(name, "wb", 1);
	        if (! out) {
	            if (errno != EISDIR) done = EXIT_ERRORS;
	            continue;
	        }
		unzzip_mem_disk_cat_file (disk, name, out);
		if (extract) fclose(out);
		break; /* match loop */
	    }
	}
    }
    return done;
} 