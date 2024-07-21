amgtar_estimate(
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
    char      *qdisk;
    amwait_t   wait_status;
    int        tarpid;
    amregex_t *rp;
    times_t    start_time;
    int        level;
    GSList    *levels;
    char      *file_exclude;
    char      *file_include;
    char      *option;

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

    qdisk = quote_string(argument->dle.disk);

    if ((option = validate_command_options(argument))) {
	fprintf(stderr, "ERROR Invalid '%s' COMMAND-OPTIONS\n", option);
	error("Invalid '%s' COMMAND-OPTIONS", option);
    }

    if (argument->calcsize) {
	char *dirname;
	int   nb_exclude;
	int   nb_include;
	char *calcsize = g_strjoin(NULL, amlibexecdir, "/", "calcsize", NULL);

	if (!check_exec_for_suid(calcsize, FALSE)) {
	    errmsg = g_strdup_printf("'%s' binary is not secure", calcsize);
	    g_free(calcsize);
	    goto common_error;
	}
	g_free(calcsize);

	if (gnutar_directory) {
	    dirname = gnutar_directory;
	} else {
	    dirname = argument->dle.device;
	}

	amgtar_build_exinclude(&argument->dle, 1,
			       &nb_exclude, &file_exclude,
			       &nb_include, &file_include);

	run_calcsize(argument->config, "GNUTAR", argument->dle.disk, dirname,
		     argument->level, file_exclude, file_include);

	if (argument->verbose == 0) {
	    if (file_exclude)
		unlink(file_exclude);
	    if (file_include)
		unlink(file_include);
        }
	return;
    }

    if (!gnutar_path) {
	errmsg = vstrallocf(_("GNUTAR-PATH not defined"));
	goto common_error;
    }

    if (!check_exec_for_suid(gnutar_path, FALSE)) {
	errmsg = g_strdup_printf("'%s' binary is not secure", gnutar_path);
	goto common_error;
    }

    if (!gnutar_listdir) {
	errmsg = vstrallocf(_("GNUTAR-LISTDIR not defined"));
	goto common_error;
    }

    for (levels = argument->level; levels != NULL; levels = levels->next) {
	level = GPOINTER_TO_INT(levels->data);
	incrname = amgtar_get_incrname(argument, level, stdout, CMD_ESTIMATE);
	cmd = stralloc(gnutar_path);
	argv_ptr = amgtar_build_argv(argument, incrname, &file_exclude,
				     &file_include, CMD_ESTIMATE);

	start_time = curclock();

	if ((nullfd = open("/dev/null", O_RDWR)) == -1) {
	    errmsg = vstrallocf(_("Cannot access /dev/null : %s"),
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
	    if (line[strlen(line)-1] == '\n') /* remove trailling \n */
		line[strlen(line)-1] = '\0';
	    if (line[0] == '\0')
		continue;
	    dbprintf("%s\n", line);
	    /* check for size match */
	    /*@ignore@*/
	    for(rp = re_table; rp->regex != NULL; rp++) {
		if(match(rp->regex, line)) {
		    if (rp->typ == DMP_SIZE) {
			size = ((the_num(line, rp->field)*rp->scale+1023.0)/1024.0);
			if(size < 0.0)
			    size = 1.0;             /* found on NeXT -- sigh */
		    }
		    break;
		}
	    }
	    /*@end@*/
	}

	while (fgets(line, sizeof(line), dumpout) != NULL) {
	    dbprintf("%s", line);
	}

	dbprintf(".....\n");
	dbprintf(_("estimate time for %s level %d: %s\n"),
		 qdisk,
		 level,
		 walltime_str(timessub(curclock(), start_time)));
	if(size == (off_t)-1) {
	    errmsg = vstrallocf(_("no size line match in %s output"), cmd);
	    dbprintf(_("%s for %s\n"), errmsg, qdisk);
	    dbprintf(".....\n");
	} else if(size == (off_t)0 && argument->level == 0) {
	    dbprintf(_("possible %s problem -- is \"%s\" really empty?\n"),
		     cmd, argument->dle.disk);
	    dbprintf(".....\n");
	}
	dbprintf(_("estimate size for %s level %d: %lld KB\n"),
		 qdisk,
		 level,
		 (long long)size);

	kill(-tarpid, SIGTERM);

	dbprintf(_("waiting for %s \"%s\" child\n"), cmd, qdisk);
	waitpid(tarpid, &wait_status, 0);
	if (WIFSIGNALED(wait_status)) {
	    errmsg = vstrallocf(_("%s terminated with signal %d: see %s"),
				cmd, WTERMSIG(wait_status), dbfn());
	} else if (WIFEXITED(wait_status)) {
	    if (exit_value[WEXITSTATUS(wait_status)] == 1) {
		errmsg = vstrallocf(_("%s exited with status %d: see %s"),
				    cmd, WEXITSTATUS(wait_status), dbfn());
	    } else {
		/* Normal exit */
	    }
	} else {
	    errmsg = vstrallocf(_("%s got bad exit: see %s"),
				cmd, dbfn());
	}
	dbprintf(_("after %s %s wait\n"), cmd, qdisk);

common_exit:
	if (errmsg) {
	    dbprintf("%s", errmsg);
	    fprintf(stdout, "ERROR %s\n", errmsg);
	}

	if (incrname) {
	    unlink(incrname);
	}

	if (argument->verbose == 0) {
	    if (file_exclude)
		unlink(file_exclude);
	    if (file_include)
		unlink(file_include);
        }

	g_ptr_array_free_full(argv_ptr);
	amfree(cmd);

	aclose(nullfd);
	afclose(dumpout);

	fprintf(stdout, "%d %lld 1\n", level, (long long)size);
    }
    amfree(qdisk);
    return;

common_error:
    qerrmsg = quote_string(errmsg);
    amfree(qdisk);
    dbprintf("%s", errmsg);
    fprintf(stdout, "ERROR %s\n", qerrmsg);
    amfree(errmsg);
    amfree(qerrmsg);
    return;
}