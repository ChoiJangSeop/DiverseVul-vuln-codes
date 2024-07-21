ambsdtar_restore(
    application_argument_t *argument)
{
    char       *cmd;
    GPtrArray  *argv_ptr = g_ptr_array_new();
    int         j;
    char       *include_filename = NULL;
    char       *exclude_filename = NULL;
    int         tarpid;
    filter_t    in_buf;
    filter_t    out_buf;
    filter_t    err_buf;
    int         tarin, tarout, tarerr;
    char       *errmsg = NULL;
    amwait_t    wait_status;

    if (!bsdtar_path) {
	error(_("BSDTAR-PATH not defined"));
    }

    if (!check_exec_for_suid(bsdtar_path, FALSE)) {
	error("'%s' binary is not secure", bsdtar_path);
    }

    cmd = g_strdup(bsdtar_path);
    g_ptr_array_add(argv_ptr, g_strdup(bsdtar_path));
    g_ptr_array_add(argv_ptr, g_strdup("--numeric-owner"));
    /* ignore trailing zero blocks on input (this was the default until tar-1.21) */
    if (argument->tar_blocksize) {
	g_ptr_array_add(argv_ptr, g_strdup("--block-size"));
	g_ptr_array_add(argv_ptr, g_strdup(argument->tar_blocksize));
    }
    g_ptr_array_add(argv_ptr, g_strdup("-xvf"));
    g_ptr_array_add(argv_ptr, g_strdup("-"));
    if (bsdtar_directory) {
	struct stat stat_buf;
	if(stat(bsdtar_directory, &stat_buf) != 0) {
	    fprintf(stderr,"can not stat directory %s: %s\n", bsdtar_directory, strerror(errno));
	    exit(1);
	}
	if (!S_ISDIR(stat_buf.st_mode)) {
	    fprintf(stderr,"%s is not a directory\n", bsdtar_directory);
	    exit(1);
	}
	if (access(bsdtar_directory, W_OK) != 0) {
	    fprintf(stderr, "Can't write to %s: %s\n", bsdtar_directory, strerror(errno));
	    exit(1);
	}
	g_ptr_array_add(argv_ptr, g_strdup("--directory"));
	g_ptr_array_add(argv_ptr, g_strdup(bsdtar_directory));
    }

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
	exclude_filename= g_strjoin(NULL, AMANDA_TMPDIR, "/", "exclude-", sdisk,  NULL);
	exclude_list = fopen(argument->dle.exclude_list->first->name, "r");
	if (!exclude_list) {
	    fprintf(stderr, "Cannot open exclude file '%s': %s\n",
		    argument->dle.exclude_list->first->name, strerror(errno));
	    error("Cannot open exclude file '%s': %s\n",
		  argument->dle.exclude_list->first->name, strerror(errno));
	    /*NOTREACHED*/
	}
	exclude = fopen(exclude_filename, "w");
	if (!exclude) {
	    fprintf(stderr, "Cannot open exclude file '%s': %s\n",
		    exclude_filename, strerror(errno));
	    fclose(exclude_list);
	    error("Cannot open exclude file '%s': %s\n",
		  exclude_filename, strerror(errno));
	    /*NOTREACHED*/
	}
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
	fclose(exclude_list);
	fclose(exclude);
	g_ptr_array_add(argv_ptr, g_strdup("--exclude-from"));
	g_ptr_array_add(argv_ptr, exclude_filename);
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
	include_filename = g_strjoin(NULL, AMANDA_TMPDIR, "/", "include-", sdisk,  NULL);
	include = fopen(include_filename, "w");
	if (!include) {
	    fprintf(stderr, "Cannot open include file '%s': %s\n",
		    include_filename, strerror(errno));
	    error("Cannot open include file '%s': %s\n",
		  include_filename, strerror(errno));
	    /*NOTREACHED*/
	}
	if (argument->dle.include_list &&
	    argument->dle.include_list->nb_element == 1) {
	    char line[2*PATH_MAX];
	    FILE *include_list = fopen(argument->dle.include_list->first->name, "r");
	    if (!include_list) {
		fclose(include);
		fprintf(stderr, "Cannot open include file '%s': %s\n",
			argument->dle.include_list->first->name,
			strerror(errno));
		error("Cannot open include file '%s': %s\n",
		      argument->dle.include_list->first->name,
		      strerror(errno));
		/*NOTREACHED*/
	    }
	    while (fgets(line, 2*PATH_MAX, include_list)) {
		char *escaped;
		line[strlen(line)-1] = '\0'; /* remove '\n' */
		if (!g_str_equal(line, ".")) {
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
	    fclose(include_list);
	}

	for (j=1; j< argument->argc; j++) {
	    if (!g_str_equal(argument->argv[j], ".")) {
		char *escaped = escape_tar_glob(argument->argv[j], &in_argv);
		if (in_argv) {
		    g_ptr_array_add(argv_include, escaped);
		} else {
		    fprintf(include,"%s\n", escaped);
		    entry_in_include++;
		    amfree(escaped);
		}
	    }
	}
	fclose(include);

	if (entry_in_include) {
	    g_ptr_array_add(argv_ptr, g_strdup("--files-from"));
	    g_ptr_array_add(argv_ptr, include_filename);
	}

	for (i = 0; i < argv_include->len; i++) {
	    g_ptr_array_add(argv_ptr, (char *)g_ptr_array_index(argv_include,i));
	}
	amfree(sdisk);
    }
    g_ptr_array_add(argv_ptr, NULL);

    debug_executing(argv_ptr);

    tarpid = pipespawnv(cmd, STDIN_PIPE|STDOUT_PIPE|STDERR_PIPE, 1,
			&tarin, &tarout, &tarerr, (char **)argv_ptr->pdata);

    in_buf.fd = 0;
    in_buf.out = tarin;
    in_buf.name = "stdin";
    in_buf.buffer = NULL;
    in_buf.first = 0;
    in_buf.size = 0;
    in_buf.allocated_size = 0;

    out_buf.fd = tarout;
    out_buf.name = "stdout";
    out_buf.buffer = NULL;
    out_buf.first = 0;
    out_buf.size = 0;
    out_buf.allocated_size = 0;

    err_buf.fd = tarerr;
    err_buf.name = "stderr";
    err_buf.buffer = NULL;
    err_buf.first = 0;
    err_buf.size = 0;
    err_buf.allocated_size = 0;

    in_buf.event = event_register((event_id_t)0, EV_READFD,
			    handle_restore_stdin, &in_buf);
    out_buf.event = event_register((event_id_t)tarout, EV_READFD,
			    handle_restore_stdout, &out_buf);
    err_buf.event = event_register((event_id_t)tarerr, EV_READFD,
			    handle_restore_stderr, &err_buf);

    event_loop(0);

    waitpid(tarpid, &wait_status, 0);
    if (WIFSIGNALED(wait_status)) {
	errmsg = g_strdup_printf(_("%s terminated with signal %d: see %s"),
				 cmd, WTERMSIG(wait_status), dbfn());
	exit_status = 1;
    } else if (WIFEXITED(wait_status) && WEXITSTATUS(wait_status) > 0) {
	errmsg = g_strdup_printf(_("%s exited with status %d: see %s"),
				 cmd, WEXITSTATUS(wait_status), dbfn());
	exit_status = 1;
    }

    g_debug(_("ambsdtar: %s: pid %ld"), cmd, (long)tarpid);
    if (errmsg) {
	g_debug("%s", errmsg);
	fprintf(stderr, "error [%s]\n", errmsg);
    }

    if (argument->verbose == 0) {
	if (exclude_filename)
	    unlink(exclude_filename);
	unlink(include_filename);
    }
    amfree(cmd);
    amfree(include_filename);
    amfree(exclude_filename);

    g_free(errmsg);
}