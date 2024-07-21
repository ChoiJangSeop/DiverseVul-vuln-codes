ambsdtar_estimate(
    application_argument_t *argument)
{
    char      *incrname = NULL;
    GPtrArray *argv_ptr;
    char      *cmd = NULL;
    int        nullfd = -1;
    int        pipefd = -1;
    FILE      *dumpout = NULL;
    off_t      size = -1;
    char       line[32768];
    char      *errmsg = NULL;
    char      *qerrmsg = NULL;
    char      *qdisk = NULL;
    amwait_t   wait_status;
    int        tarpid;
    amregex_t *rp;
    times_t    start_time;
    int        level;
    GSList    *levels;
    char      *file_exclude = NULL;
    char      *file_include = NULL;
    GString   *strbuf;

    if (!argument->level) {
        fprintf(stderr, "ERROR No level argument\n");
        error(_("No level argument"));
    }
    if (!argument->dle.disk) {
        fprintf(stderr, "ERROR No disk argument\n");
        error(_("No disk argument"));
    }
    if (!argument->dle.device) {
        fprintf(stderr, "ERROR No device argument\n");
        error(_("No device argument"));
    }

    if (argument->calcsize) {
	char *dirname;
	int   nb_exclude;
	int   nb_include;
	char *calcsize = g_strjoin(NULL, amlibexecdir, "/", "calcsize", NULL);

	if (!check_exec_for_suid(calcsize, FALSE)) {
	    errmsg = g_strdup_printf("'%s' binary is not secure", calcsize);
	    goto common_error;
	    return;
	}

	if (bsdtar_directory) {
	    dirname = bsdtar_directory;
	} else {
	    dirname = argument->dle.device;
	}
	ambsdtar_build_exinclude(&argument->dle, 1,
				 &nb_exclude, &file_exclude,
				 &nb_include, &file_include);

	run_calcsize(argument->config, "BSDTAR", argument->dle.disk, dirname,
		     argument->level, file_exclude, file_include);

	if (argument->verbose == 0) {
	    if (file_exclude)
		unlink(file_exclude);
	    if (file_include)
		unlink(file_include);
	}
	amfree(file_exclude);
	amfree(file_include);
	amfree(qdisk);
	return;
    }

    if (!bsdtar_path) {
	errmsg = g_strdup(_("BSDTAR-PATH not defined"));
	goto common_error;
    }

    if (!check_exec_for_suid(bsdtar_path, FALSE)) {
	errmsg = g_strdup_printf("'%s' binary is not secure", bsdtar_path);
	goto common_error;
    }

    if (!state_dir) {
	errmsg = g_strdup(_("STATE-DIR not defined"));
	goto common_error;
    }

    qdisk = quote_string(argument->dle.disk);
    for (levels = argument->level; levels != NULL; levels = levels->next) {
	char *timestamps;
	level = GPOINTER_TO_INT(levels->data);
	timestamps = ambsdtar_get_timestamps(argument, level, stdout, CMD_ESTIMATE);
	cmd = g_strdup(bsdtar_path);
	argv_ptr = ambsdtar_build_argv(argument, timestamps, &file_exclude,
				       &file_include, CMD_ESTIMATE);
	amfree(timestamps);

	start_time = curclock();

	if ((nullfd = open("/dev/null", O_RDWR)) == -1) {
	    errmsg = g_strdup_printf(_("Cannot access /dev/null : %s"),
				strerror(errno));
	    goto common_exit;
	}

	tarpid = pipespawnv(cmd, STDERR_PIPE, 1,
			    &nullfd, &nullfd, &pipefd,
			    (char **)argv_ptr->pdata);

	dumpout = fdopen(pipefd,"r");
	if (!dumpout) {
	    error(_("Can't fdopen: %s"), strerror(errno));
	    /*NOTREACHED*/
	}

	size = (off_t)-1;
	while (size < 0 && (fgets(line, sizeof(line), dumpout) != NULL)) {
	    if (strlen(line) > 0 && line[strlen(line)-1] == '\n') {
		/* remove trailling \n */
		line[strlen(line)-1] = '\0';
	    }
	    if (line[0] == '\0')
		continue;
	    g_debug("%s", line);
	    /* check for size match */
	    /*@ignore@*/
	    for(rp = re_table; rp->regex != NULL; rp++) {
		if(match(rp->regex, line)) {
		    if (rp->typ == DMP_SIZE) {
			off_t blocksize = gblocksize;
			size = ((the_num(line, rp->field)*rp->scale+1023.0)/1024.0);
			if(size < 0.0)
			    size = 1.0;             /* found on NeXT -- sigh */
			if (!blocksize) {
			    blocksize = 20;
			}
			blocksize /= 2;
			size = (size+blocksize-1) / blocksize;
			size *= blocksize;
		    }
		    break;
		}
	    }
	    /*@end@*/
	}

	while (fgets(line, sizeof(line), dumpout) != NULL) {
	    g_debug("%s", line);
	}

	g_debug(".....");
	g_debug(_("estimate time for %s level %d: %s"),
		 qdisk,
		 level,
		 walltime_str(timessub(curclock(), start_time)));
	if(size == (off_t)-1) {
	    errmsg = g_strdup_printf(_("no size line match in %s output"), cmd);
	    g_debug(_("%s for %s"), errmsg, qdisk);
	    g_debug(".....");
	} else if(size == (off_t)0 && argument->level == 0) {
	    g_debug(_("possible %s problem -- is \"%s\" really empty?"),
		     cmd, argument->dle.disk);
	    g_debug(".....");
	}
	g_debug(_("estimate size for %s level %d: %lld KB"),
		 qdisk,
		 level,
		 (long long)size);

	(void)kill(-tarpid, SIGTERM);

	g_debug(_("waiting for %s \"%s\" child"), cmd, qdisk);
	waitpid(tarpid, &wait_status, 0);
	if (WIFSIGNALED(wait_status)) {
	    strbuf = g_string_new(errmsg);
	    g_string_append_printf(strbuf, "%s terminated with signal %d: see %s",
                cmd, WTERMSIG(wait_status), dbfn());
	    g_free(errmsg);
            errmsg = g_string_free(strbuf, FALSE);
	    exit_status = 1;
	} else if (WIFEXITED(wait_status)) {
	    if (exit_value[WEXITSTATUS(wait_status)] == 1) {
                strbuf = g_string_new(errmsg);
                g_string_append_printf(strbuf, "%s exited with status %d: see %s",
                    cmd, WEXITSTATUS(wait_status), dbfn());
		g_free(errmsg);
                errmsg = g_string_free(strbuf, FALSE);
		exit_status = 1;
	    } else {
		/* Normal exit */
	    }
	} else {
	    errmsg = g_strdup_printf(_("%s got bad exit: see %s"),
				cmd, dbfn());
	    exit_status = 1;
	}
	g_debug(_("after %s %s wait"), cmd, qdisk);

common_exit:
	if (errmsg) {
	    g_debug("%s", errmsg);
	    fprintf(stdout, "ERROR %s\n", errmsg);
	    amfree(errmsg);
	}

	if (argument->verbose == 0) {
	    if (file_exclude)
		unlink(file_exclude);
	    if (file_include)
		unlink(file_include);
        }

	g_ptr_array_free_full(argv_ptr);
	amfree(cmd);
	amfree(incrname);

	aclose(nullfd);
	afclose(dumpout);

	fprintf(stdout, "%d %lld 1\n", level, (long long)size);
    }
    amfree(qdisk);
    amfree(file_exclude);
    amfree(file_include);
    amfree(errmsg);
    amfree(qerrmsg);
    amfree(incrname);
    return;

common_error:
    qerrmsg = quote_string(errmsg);
    amfree(qdisk);
    g_debug("%s", errmsg);
    fprintf(stdout, "ERROR %s\n", qerrmsg);
    exit_status = 1;
    amfree(file_exclude);
    amfree(file_include);
    amfree(errmsg);
    amfree(qerrmsg);
    amfree(incrname);
    return;
}