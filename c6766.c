getsize_gnutar(
    dle_t      *dle,
    int		level,
    time_t	dumpsince,
    char      **errmsg)
{
    int pipefd = -1, nullfd = -1;
    pid_t dumppid;
    off_t size = (off_t)-1;
    FILE *dumpout = NULL;
    char *incrname = NULL;
    char *basename = NULL;
    char *dirname = NULL;
    char *inputname = NULL;
    FILE *in = NULL;
    FILE *out = NULL;
    char *line = NULL;
    char *cmd = NULL;
    char *command = NULL;
    char dumptimestr[80];
    struct tm *gmtm;
    int nb_exclude = 0;
    int nb_include = 0;
    GPtrArray *argv_ptr = g_ptr_array_new();
    char *file_exclude = NULL;
    char *file_include = NULL;
    times_t start_time;
    int infd, outfd;
    ssize_t nb;
    char buf[32768];
    char *qdisk = quote_string(dle->disk);
    char *gnutar_list_dir;
    amwait_t wait_status;
    char tmppath[PATH_MAX];

    if (level > 9)
	return -2; /* planner will not even consider this level */

    if(dle->exclude_file) nb_exclude += dle->exclude_file->nb_element;
    if(dle->exclude_list) nb_exclude += dle->exclude_list->nb_element;
    if(dle->include_file) nb_include += dle->include_file->nb_element;
    if(dle->include_list) nb_include += dle->include_list->nb_element;

    if(nb_exclude > 0) file_exclude = build_exclude(dle, 0);
    if(nb_include > 0) file_include = build_include(dle, 0);

    gnutar_list_dir = getconf_str(CNF_GNUTAR_LIST_DIR);
    if (strlen(gnutar_list_dir) == 0)
	gnutar_list_dir = NULL;
    if (gnutar_list_dir) {
	char number[NUM_STR_SIZE];
	int baselevel;
	char *sdisk = sanitise_filename(dle->disk);

	basename = vstralloc(gnutar_list_dir,
			     "/",
			     g_options->hostname,
			     sdisk,
			     NULL);
	amfree(sdisk);

	g_snprintf(number, SIZEOF(number), "%d", level);
	incrname = vstralloc(basename, "_", number, ".new", NULL);
	unlink(incrname);

	/*
	 * Open the listed incremental file from the previous level.  Search
	 * backward until one is found.  If none are found (which will also
	 * be true for a level 0), arrange to read from /dev/null.
	 */
	baselevel = level;
	infd = -1;
	while (infd == -1) {
	    if (--baselevel >= 0) {
		g_snprintf(number, SIZEOF(number), "%d", baselevel);
		inputname = newvstralloc(inputname,
					 basename, "_", number, NULL);
	    } else {
		inputname = newstralloc(inputname, "/dev/null");
	    }
	    if ((infd = open(inputname, O_RDONLY)) == -1) {

		*errmsg = vstrallocf(_("gnutar: error opening %s: %s"),
				     inputname, strerror(errno));
		dbprintf("%s\n", *errmsg);
		if (baselevel < 0) {
		    goto common_exit;
		}
		amfree(*errmsg);
	    }
	}

	/*
	 * Copy the previous listed incremental file to the new one.
	 */
	if ((outfd = open(incrname, O_WRONLY|O_CREAT, 0600)) == -1) {
	    *errmsg = vstrallocf(_("opening %s: %s"),
			         incrname, strerror(errno));
	    dbprintf("%s\n", *errmsg);
	    goto common_exit;
	}

	while ((nb = read(infd, &buf, SIZEOF(buf))) > 0) {
	    if (full_write(outfd, &buf, (size_t)nb) < (size_t)nb) {
		*errmsg = vstrallocf(_("writing to %s: %s"),
				     incrname, strerror(errno));
		dbprintf("%s\n", *errmsg);
		goto common_exit;
	    }
	}

	if (nb < 0) {
	    *errmsg = vstrallocf(_("reading from %s: %s"),
			         inputname, strerror(errno));
	    dbprintf("%s\n", *errmsg);
	    goto common_exit;
	}

	if (close(infd) != 0) {
	    *errmsg = vstrallocf(_("closing %s: %s"),
			         inputname, strerror(errno));
	    dbprintf("%s\n", *errmsg);
	    goto common_exit;
	}
	if (close(outfd) != 0) {
	    *errmsg = vstrallocf(_("closing %s: %s"),
			         incrname, strerror(errno));
	    dbprintf("%s\n", *errmsg);
	    goto common_exit;
	}

	amfree(inputname);
	amfree(basename);
    }

    gmtm = gmtime(&dumpsince);
    g_snprintf(dumptimestr, SIZEOF(dumptimestr),
		"%04d-%02d-%02d %2d:%02d:%02d GMT",
		gmtm->tm_year + 1900, gmtm->tm_mon+1, gmtm->tm_mday,
		gmtm->tm_hour, gmtm->tm_min, gmtm->tm_sec);

    dirname = amname_to_dirname(dle->device);

    cmd = vstralloc(amlibexecdir, "/", "runtar", NULL);
    g_ptr_array_add(argv_ptr, stralloc("runtar"));
    if (g_options->config)
	g_ptr_array_add(argv_ptr, stralloc(g_options->config));
    else
	g_ptr_array_add(argv_ptr, stralloc("NOCONFIG"));

#ifdef GNUTAR
    g_ptr_array_add(argv_ptr, stralloc(GNUTAR));
#else
    g_ptr_array_add(argv_ptr, stralloc("tar"));
#endif
    g_ptr_array_add(argv_ptr, stralloc("--create"));
    g_ptr_array_add(argv_ptr, stralloc("--file"));
    g_ptr_array_add(argv_ptr, stralloc("/dev/null"));
    /* use --numeric-owner for estimates, to reduce the number of user/group
     * lookups required */
    g_ptr_array_add(argv_ptr, stralloc("--numeric-owner"));
    g_ptr_array_add(argv_ptr, stralloc("--directory"));
    canonicalize_pathname(dirname, tmppath);
    g_ptr_array_add(argv_ptr, stralloc(tmppath));
    g_ptr_array_add(argv_ptr, stralloc("--one-file-system"));
    if (gnutar_list_dir) {
	g_ptr_array_add(argv_ptr, stralloc("--listed-incremental"));
	g_ptr_array_add(argv_ptr, stralloc(incrname));
    } else {
	g_ptr_array_add(argv_ptr, stralloc("--incremental"));
	g_ptr_array_add(argv_ptr, stralloc("--newer"));
	g_ptr_array_add(argv_ptr, stralloc(dumptimestr));
    }
#ifdef ENABLE_GNUTAR_ATIME_PRESERVE
    /* --atime-preserve causes gnutar to call
     * utime() after reading files in order to
     * adjust their atime.  However, utime()
     * updates the file's ctime, so incremental
     * dumps will think the file has changed. */
    g_ptr_array_add(argv_ptr, stralloc("--atime-preserve"));
#endif
    g_ptr_array_add(argv_ptr, stralloc("--sparse"));
    g_ptr_array_add(argv_ptr, stralloc("--ignore-failed-read"));
    g_ptr_array_add(argv_ptr, stralloc("--totals"));

    if(file_exclude) {
	g_ptr_array_add(argv_ptr, stralloc("--exclude-from"));
	g_ptr_array_add(argv_ptr, stralloc(file_exclude));
    }

    if(file_include) {
	g_ptr_array_add(argv_ptr, stralloc("--files-from"));
	g_ptr_array_add(argv_ptr, stralloc(file_include));
    }
    else {
	g_ptr_array_add(argv_ptr, stralloc("."));
    }
    g_ptr_array_add(argv_ptr, NULL);

    start_time = curclock();

    if ((nullfd = open("/dev/null", O_RDWR)) == -1) {
	*errmsg = vstrallocf(_("Cannot access /dev/null : %s"),
			     strerror(errno));
	dbprintf("%s\n", *errmsg);
	goto common_exit;
    }

    command = (char *)g_ptr_array_index(argv_ptr, 0);
    dumppid = pipespawnv(cmd, STDERR_PIPE, 0,
			 &nullfd, &nullfd, &pipefd, (char **)argv_ptr->pdata);

    dumpout = fdopen(pipefd,"r");
    if (!dumpout) {
	error(_("Can't fdopen: %s"), strerror(errno));
	/*NOTREACHED*/
    }

    for(size = (off_t)-1; (line = agets(dumpout)) != NULL; free(line)) {
	if (line[0] == '\0')
	    continue;
	dbprintf("%s\n", line);
	size = handle_dumpline(line);
	if(size > (off_t)-1) {
	    amfree(line);
	    while ((line = agets(dumpout)) != NULL) {
		if (line[0] != '\0') {
		    break;
		}
		amfree(line);
	    }
	    if (line != NULL) {
		dbprintf("%s\n", line);
		break;
	    }
	    break;
	}
    }
    amfree(line);

    dbprintf(".....\n");
    dbprintf(_("estimate time for %s level %d: %s\n"),
	      qdisk,
	      level,
	      walltime_str(timessub(curclock(), start_time)));
    if(size == (off_t)-1) {
	*errmsg = vstrallocf(_("no size line match in %s output"),
			     command);
	dbprintf(_("%s for %s\n"), *errmsg, qdisk);
	dbprintf(".....\n");
    } else if(size == (off_t)0 && level == 0) {
	dbprintf(_("possible %s problem -- is \"%s\" really empty?\n"),
		  command, dle->disk);
	dbprintf(".....\n");
    }
    dbprintf(_("estimate size for %s level %d: %lld KB\n"),
	      qdisk,
	      level,
	      (long long)size);

    kill(-dumppid, SIGTERM);

    dbprintf(_("waiting for %s \"%s\" child\n"),
	     command, qdisk);
    waitpid(dumppid, &wait_status, 0);
    if (WIFSIGNALED(wait_status)) {
	*errmsg = vstrallocf(_("%s terminated with signal %d: see %s"),
			     cmd, WTERMSIG(wait_status), dbfn());
    } else if (WIFEXITED(wait_status)) {
	if (WEXITSTATUS(wait_status) != 0) {
	    *errmsg = vstrallocf(_("%s exited with status %d: see %s"),
			         cmd, WEXITSTATUS(wait_status), dbfn());
	} else {
	    /* Normal exit */
	}
    } else {
	*errmsg = vstrallocf(_("%s got bad exit: see %s"),
			     cmd, dbfn());
    }
    dbprintf(_("after %s %s wait\n"), command, qdisk);

common_exit:

    if (incrname) {
	unlink(incrname);
    }
    amfree(incrname);
    amfree(basename);
    amfree(dirname);
    amfree(inputname);
    g_ptr_array_free_full(argv_ptr);
    amfree(qdisk);
    amfree(cmd);
    amfree(file_exclude);
    amfree(file_include);

    aclose(nullfd);
    afclose(dumpout);
    afclose(in);
    afclose(out);

    return size;
}