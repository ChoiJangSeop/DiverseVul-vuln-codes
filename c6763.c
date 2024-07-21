amgtar_restore(
    application_argument_t *argument)
{
    char       *cmd;
    GPtrArray  *argv_ptr = g_ptr_array_new();
    char      **env;
    int         j;
    char       *e;
    char       *include_filename = NULL;
    char       *exclude_filename = NULL;

    if (!gnutar_path) {
	error(_("GNUTAR-PATH not defined"));
    }

    if (!check_exec_for_suid(gnutar_path, FALSE)) {
        error("'%s' binary is not secure", gnutar_path);
    }

    cmd = stralloc(gnutar_path);
    g_ptr_array_add(argv_ptr, stralloc(gnutar_path));
    g_ptr_array_add(argv_ptr, stralloc("--numeric-owner"));
    if (gnutar_no_unquote)
	g_ptr_array_add(argv_ptr, stralloc("--no-unquote"));
    if (gnutar_acls)
	g_ptr_array_add(argv_ptr, stralloc("--acls"));
    if (gnutar_selinux)
	g_ptr_array_add(argv_ptr, stralloc("--selinux"));
    if (gnutar_xattrs)
	g_ptr_array_add(argv_ptr, stralloc("--xattrs"));
    /* ignore trailing zero blocks on input (this was the default until tar-1.21) */
    if (argument->ignore_zeros) {
	g_ptr_array_add(argv_ptr, stralloc("--ignore-zeros"));
    }
    if (argument->tar_blocksize) {
	g_ptr_array_add(argv_ptr, stralloc("--blocking-factor"));
	g_ptr_array_add(argv_ptr, stralloc(argument->tar_blocksize));
    }
    g_ptr_array_add(argv_ptr, stralloc("-xpGvf"));
    g_ptr_array_add(argv_ptr, stralloc("-"));
    if (gnutar_directory) {
	struct stat stat_buf;
	if(stat(gnutar_directory, &stat_buf) != 0) {
	    fprintf(stderr,"can not stat directory %s: %s\n", gnutar_directory, strerror(errno));
	    exit(1);
	}
	if (!S_ISDIR(stat_buf.st_mode)) {
	    fprintf(stderr,"%s is not a directory\n", gnutar_directory);
	    exit(1);
	}
	if (access(gnutar_directory, W_OK) != 0) {
	    fprintf(stderr, "Can't write to %s: %s\n", gnutar_directory, strerror(errno));
	    exit(1);
	}
	g_ptr_array_add(argv_ptr, stralloc("--directory"));
	g_ptr_array_add(argv_ptr, stralloc(gnutar_directory));
    }

    g_ptr_array_add(argv_ptr, stralloc("--wildcards"));
    if (argument->dle.exclude_list &&
	argument->dle.exclude_list->nb_element == 1) {
	FILE      *exclude;
	char      *sdisk;
	int        in_argv;
	int        entry_in_exclude = 0;
	char       line[2*PATH_MAX];
	FILE      *exclude_list;

	if (argument->dle.disk) {
	    sdisk = sanitise_filename(argument->dle.disk);
	} else {
	    sdisk = g_strdup_printf("no_dle-%d", (int)getpid());
	}
	exclude_filename= vstralloc(AMANDA_TMPDIR, "/", "exclude-", sdisk,  NULL);
	exclude_list = fopen(argument->dle.exclude_list->first->name, "r");

	exclude = fopen(exclude_filename, "w");
	while (fgets(line, 2*PATH_MAX, exclude_list)) {
	    char *escaped;
	    line[strlen(line)-1] = '\0'; /* remove '\n' */
	    escaped = escape_tar_glob(line, &in_argv);
	    if (in_argv) {
		g_ptr_array_add(argv_ptr, "--exclude");
		g_ptr_array_add(argv_ptr, escaped);
	    } else {
		fprintf(exclude,"%s\n", escaped);
		entry_in_exclude++;
		amfree(escaped);
	    }
	}
	fclose(exclude);
	g_ptr_array_add(argv_ptr, stralloc("--exclude-from"));
	g_ptr_array_add(argv_ptr, exclude_filename);
    }

    if (argument->exclude_list_glob) {
	g_ptr_array_add(argv_ptr, stralloc("--exclude-from"));
	g_ptr_array_add(argv_ptr, stralloc(argument->exclude_list_glob));
    }

    {
	GPtrArray *argv_include = g_ptr_array_new();
	FILE      *include;
	char      *sdisk;
	int        in_argv;
	guint      i;
	int        entry_in_include = 0;

	if (argument->dle.disk) {
	    sdisk = sanitise_filename(argument->dle.disk);
	} else {
	    sdisk = g_strdup_printf("no_dle-%d", (int)getpid());
	}
	include_filename = vstralloc(AMANDA_TMPDIR, "/", "include-", sdisk,  NULL);
	include = fopen(include_filename, "w");
	if (argument->dle.include_list &&
	    argument->dle.include_list->nb_element == 1) {
	    char line[2*PATH_MAX];
	    FILE *include_list = fopen(argument->dle.include_list->first->name, "r");
	    while (fgets(line, 2*PATH_MAX, include_list)) {
		char *escaped;
		line[strlen(line)-1] = '\0'; /* remove '\n' */
		escaped = escape_tar_glob(line, &in_argv);
		if (in_argv) {
		    g_ptr_array_add(argv_include, escaped);
		} else {
		    fprintf(include,"%s\n", escaped);
		    entry_in_include++;
		    amfree(escaped);
		}
	    }
	}

	for (j=1; j< argument->argc; j++) {
	    char *escaped = escape_tar_glob(argument->argv[j], &in_argv);
	    if (in_argv) {
		g_ptr_array_add(argv_include, escaped);
	    } else {
		fprintf(include,"%s\n", escaped);
		entry_in_include++;
		amfree(escaped);
	    }
	}
	fclose(include);

	if (entry_in_include) {
	    g_ptr_array_add(argv_ptr, stralloc("--files-from"));
	    g_ptr_array_add(argv_ptr, include_filename);
	}

	if (argument->include_list_glob) {
	    g_ptr_array_add(argv_ptr, stralloc("--files-from"));
	    g_ptr_array_add(argv_ptr, stralloc(argument->include_list_glob));
	}

	for (i = 0; i < argv_include->len; i++) {
	    g_ptr_array_add(argv_ptr, (char *)g_ptr_array_index(argv_include,i));
	}
    }
    g_ptr_array_add(argv_ptr, NULL);

    debug_executing(argv_ptr);

    env = safe_env();
    become_root();
    execve(cmd, (char **)argv_ptr->pdata, env);
    e = strerror(errno);
    error(_("error [exec %s: %s]"), cmd, e);
}