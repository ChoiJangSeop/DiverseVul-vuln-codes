unzzip_list (int argc, char ** argv, int verbose)
{
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
    {  /* list all */
	ZZIP_MEM_ENTRY* entry = zzip_mem_disk_findfirst(disk);
	DBG2("findfirst %p", entry);
	for (; entry ; entry = zzip_mem_disk_findnext(disk, entry))
	{
	    char* name = zzip_mem_entry_to_name (entry);
	    long long usize = entry->zz_usize;
	    if (!verbose)
	    {
		printf ("%22lli %s\n", usize, name);
	    } else 
	    {
		long long csize = entry->zz_csize;
		unsigned compr = entry->zz_compr;
        	const char* defl = (compr < sizeof(comprlevel)) ? comprlevel[compr] : "(redu)";
		printf ("%lli/%lli %s %s\n", csize, usize, defl, name);
	    }
	}
	return 0;
    }

    if (argc == 3)
    {  /* list from one spec */
	ZZIP_MEM_ENTRY* entry = 0;
	while ((entry = zzip_mem_disk_findmatch(disk, argv[2], entry, 0, 0)))
	{
	    char* name = zzip_mem_entry_to_name (entry);
	    long long usize = entry->zz_usize;
	    if (!verbose)
	    {
		printf ("%22lli %s\n", usize, name);
	    } else 
	    {
		long long csize = entry->zz_csize;
		unsigned compr = entry->zz_compr;
		const char* defl = (compr < sizeof(comprlevel)) ? comprlevel[compr] : "(redu)";
		printf ("%lli/%lli %s %s\n", csize, usize, defl, name);
	    }
	}
	return 0;
    }

    {   /* list only the matching entries - in order of zip directory */
	ZZIP_MEM_ENTRY* entry = zzip_mem_disk_findfirst(disk);
	for (; entry ; entry = zzip_mem_disk_findnext(disk, entry))
	{
	    char* name = zzip_mem_entry_to_name (entry);
	    for (argn=1; argn < argc; argn++)
	    {
		if (! _zzip_fnmatch (argv[argn], name, 
		      _zzip_FNM_NOESCAPE|_zzip_FNM_PATHNAME|_zzip_FNM_PERIOD))
		{
		    char* name = zzip_mem_entry_to_name (entry);
		    long long usize = entry->zz_usize;
		    if (!verbose)
		    {
			printf ("%22lli %s\n", usize, name);
		    } else 
		    {
			long long csize = entry->zz_csize;
			unsigned compr = entry->zz_compr;
			const char* defl = (compr < sizeof(comprlevel)) ? comprlevel[compr] : "(redu)";
	    		printf ("%lli/%lli %s %s\n", csize, usize, defl, name);
	    	    }
		    break; /* match loop */
		}
	    }
	}
	return 0;
    }
} 