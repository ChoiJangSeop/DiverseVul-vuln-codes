ambsdtar_build_argv(
    application_argument_t *argument,
    char  *timestamps,
    char **file_exclude,
    char **file_include,
    int    command)
{
    int        nb_exclude = 0;
    int        nb_include = 0;
    char      *dirname;
    char       tmppath[PATH_MAX];
    GPtrArray *argv_ptr = g_ptr_array_new();
    GSList    *copt;

    ambsdtar_build_exinclude(&argument->dle, 1,
			     &nb_exclude, file_exclude,
			     &nb_include, file_include);

    if (bsdtar_directory) {
	dirname = bsdtar_directory;
    } else {
	dirname = argument->dle.device;
    }

    g_ptr_array_add(argv_ptr, g_strdup(bsdtar_path));

    g_ptr_array_add(argv_ptr, g_strdup("--create"));
    g_ptr_array_add(argv_ptr, g_strdup("--file"));
    if (command == CMD_ESTIMATE) {
        g_ptr_array_add(argv_ptr, g_strdup("/dev/null"));
    } else {
        g_ptr_array_add(argv_ptr, g_strdup("-"));
    }
    g_ptr_array_add(argv_ptr, g_strdup("--directory"));
    canonicalize_pathname(dirname, tmppath);
    g_ptr_array_add(argv_ptr, g_strdup(tmppath));
    if (timestamps) {
	g_ptr_array_add(argv_ptr, g_strdup("--newer"));
	g_ptr_array_add(argv_ptr, g_strdup(timestamps));
    }
    if (bsdtar_onefilesystem)
	g_ptr_array_add(argv_ptr, g_strdup("--one-file-system"));
    if (argument->tar_blocksize) {
	g_ptr_array_add(argv_ptr, g_strdup("--block-size"));
	g_ptr_array_add(argv_ptr, g_strdup(argument->tar_blocksize));
    }
    g_ptr_array_add(argv_ptr, g_strdup("--totals"));

    for (copt = argument->command_options; copt != NULL; copt = copt->next) {
	g_ptr_array_add(argv_ptr, g_strdup((char *)copt->data));
    }

    if (*file_exclude) {
	g_ptr_array_add(argv_ptr, g_strdup("--exclude-from"));
	g_ptr_array_add(argv_ptr, g_strdup(*file_exclude));
    }

    if (*file_include) {
	g_ptr_array_add(argv_ptr, g_strdup("--files-from"));
	g_ptr_array_add(argv_ptr, g_strdup(*file_include));
    } else {
	g_ptr_array_add(argv_ptr, g_strdup("."));
    }
    g_ptr_array_add(argv_ptr, NULL);

    return(argv_ptr);
}