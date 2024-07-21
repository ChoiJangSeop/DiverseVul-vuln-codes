GPtrArray *amgtar_build_argv(
    application_argument_t *argument,
    char  *incrname,
    char **file_exclude,
    char **file_include,
    int    command)
{
    int    nb_exclude;
    int    nb_include;
    char  *dirname;
    char   tmppath[PATH_MAX];
    GPtrArray *argv_ptr = g_ptr_array_new();
    GSList    *copt;

    check_no_check_device();
    amgtar_build_exinclude(&argument->dle, 1,
			   &nb_exclude, file_exclude,
			   &nb_include, file_include);

    if (gnutar_directory) {
	dirname = gnutar_directory;
    } else {
	dirname = argument->dle.device;
    }

    g_ptr_array_add(argv_ptr, stralloc(gnutar_path));

    g_ptr_array_add(argv_ptr, stralloc("--create"));
    if (command == CMD_BACKUP && argument->dle.create_index)
        g_ptr_array_add(argv_ptr, stralloc("--verbose"));
    g_ptr_array_add(argv_ptr, stralloc("--file"));
    if (command == CMD_ESTIMATE) {
        g_ptr_array_add(argv_ptr, stralloc("/dev/null"));
    } else {
        g_ptr_array_add(argv_ptr, stralloc("-"));
    }
    if (gnutar_no_unquote)
	g_ptr_array_add(argv_ptr, stralloc("--no-unquote"));
    g_ptr_array_add(argv_ptr, stralloc("--directory"));
    canonicalize_pathname(dirname, tmppath);
    g_ptr_array_add(argv_ptr, stralloc(tmppath));
    if (gnutar_onefilesystem)
	g_ptr_array_add(argv_ptr, stralloc("--one-file-system"));
    if (gnutar_atimepreserve)
	g_ptr_array_add(argv_ptr, stralloc("--atime-preserve=system"));
    if (!gnutar_checkdevice)
	g_ptr_array_add(argv_ptr, stralloc("--no-check-device"));
    if (gnutar_acls)
	g_ptr_array_add(argv_ptr, stralloc("--acls"));
    if (gnutar_selinux)
	g_ptr_array_add(argv_ptr, stralloc("--selinux"));
    if (gnutar_xattrs)
	g_ptr_array_add(argv_ptr, stralloc("--xattrs"));
    g_ptr_array_add(argv_ptr, stralloc("--listed-incremental"));
    g_ptr_array_add(argv_ptr, stralloc(incrname));
    if (gnutar_sparse) {
	if (!gnutar_sparse_set) {
	    char  *gtar_version;
	    char  *minor_version;
	    char  *sminor_version;
	    char  *gv;
	    int    major;
	    int    minor;
	    GPtrArray *version_ptr = g_ptr_array_new();

	    g_ptr_array_add(version_ptr, gnutar_path);
	    g_ptr_array_add(version_ptr, "--version");
	    g_ptr_array_add(version_ptr, NULL);
	    gtar_version = get_first_line(version_ptr);
	    if (gtar_version) {
		for (gv = gtar_version; *gv && !g_ascii_isdigit(*gv); gv++);
		minor_version = index(gtar_version, '.');
		if (minor_version) {
		    *minor_version++ = '\0';
		    sminor_version = index(minor_version, '.');
		    if (sminor_version) {
			*sminor_version = '\0';
		    }
		    major = atoi(gv);
		    minor = atoi(minor_version);
		    if (major < 1 ||
			(major == 1 && minor < 28)) {
			gnutar_sparse = 0;
		    }
		}
	    }
	    g_ptr_array_free(version_ptr, TRUE);
	    amfree(gtar_version);
	}
	if (gnutar_sparse) {
	    g_ptr_array_add(argv_ptr, g_strdup("--sparse"));
	}
    }
    if (argument->tar_blocksize) {
	g_ptr_array_add(argv_ptr, stralloc("--blocking-factor"));
	g_ptr_array_add(argv_ptr, stralloc(argument->tar_blocksize));
    }
    g_ptr_array_add(argv_ptr, stralloc("--ignore-failed-read"));
    g_ptr_array_add(argv_ptr, stralloc("--totals"));

    for (copt = argument->command_options; copt != NULL; copt = copt->next) {
	g_ptr_array_add(argv_ptr, stralloc((char *)copt->data));
    }

    if (*file_exclude) {
	g_ptr_array_add(argv_ptr, stralloc("--exclude-from"));
	g_ptr_array_add(argv_ptr, stralloc(*file_exclude));
    }

    if (*file_include) {
	g_ptr_array_add(argv_ptr, stralloc("--files-from"));
	g_ptr_array_add(argv_ptr, stralloc(*file_include));
    }
    else {
	g_ptr_array_add(argv_ptr, stralloc("."));
    }
    g_ptr_array_add(argv_ptr, NULL);

    return(argv_ptr);
}