amstar_estimate(
    application_argument_t *argument)
{
    GPtrArray  *argv_ptr;
    char       *cmd = NULL;
    int         nullfd;
    int         pipefd;
    FILE       *dumpout = NULL;
    off_t       size = -1;
    char        line[32768];
    char       *errmsg = NULL;
    char       *qerrmsg;
    char       *qdisk;
    amwait_t    wait_status;
    int         starpid;
    amregex_t  *rp;
    times_t     start_time;
    int         level = 0;
    GSList     *levels = NULL;
    char       *option;

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

    if (argument->dle.include_list &&
	argument->dle.include_list->nb_element >= 0) {
	fprintf(stderr, "ERROR include-list not supported for backup\n");
    }

    if (check_device(argument) == 0) {
	return;
    }

    if ((option = validate_command_options(argument))) {
	fprintf(stderr, "ERROR Invalid '%s' COMMAND-OPTIONS\n", option);
	error("Invalid '%s' COMMAND-OPTIONS", option);
    }

    qdisk = quote_string(argument->dle.disk);
    if (argument->calcsize) {
	char *dirname;
	char *calcsize = g_strjoin(NULL, amlibexecdir, "/", "calcsize", NULL);
	if (!check_exec_for_suid(calcsize, FALSE)) {
	    errmsg = g_strdup_printf("'%s' binary is not secure", calcsize);
	    goto common_error;
	    return;
	}

	if (star_directory) {
	    dirname = star_directory;
	} else {
	    dirname = argument->dle.device;
	}
	run_calcsize(argument->config, "STAR", argument->dle.disk, dirname,
		     argument->level, NULL, NULL);
	return;
    }

    if (!star_path) {
	errmsg = vstrallocf(_("STAR-PATH not defined"));
	goto common_error;
    }
    if (!check_exec_for_suid(star_path, FALSE)) {
	errmsg = g_strdup_printf("'%s' binary is not secure", star_path);
	goto common_error;
    }

    cmd = stralloc(star_path);

    start_time = curclock();

    for (levels = argument->level; levels != NULL; levels = levels->next) {
	level = GPOINTER_TO_INT(levels->data);
	argv_ptr = amstar_build_argv(argument, level, CMD_ESTIMATE, NULL);

	if ((nullfd = open("/dev/null", O_RDWR)) == -1) {
	    errmsg = vstrallocf(_("Cannot access /dev/null : %s"),
				strerror(errno));
	    goto common_error;
	}

	starpid = pipespawnv(cmd, STDERR_PIPE, 1,
			     &nullfd, &nullfd, &pipefd,
			     (char **)argv_ptr->pdata);

	dumpout = fdopen(pipefd,"r");
	if (!dumpout) {
	    errmsg = vstrallocf(_("Can't fdopen: %s"), strerror(errno));
	    goto common_error;
	}

	size = (off_t)-1;
	while (size < 0 && (fgets(line, sizeof(line), dumpout)) != NULL) {
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

	while ((fgets(line, sizeof(line), dumpout)) != NULL) {
	    dbprintf("%s", line);
	}

	dbprintf(".....\n");
	dbprintf(_("estimate time for %s level %d: %s\n"),
		 qdisk,
		 level,
		 walltime_str(timessub(curclock(), start_time)));
	if(size == (off_t)-1) {
	    errmsg = vstrallocf(_("no size line match in %s output"),
				cmd);
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

	kill(-starpid, SIGTERM);

	dbprintf(_("waiting for %s \"%s\" child\n"), cmd, qdisk);
	waitpid(starpid, &wait_status, 0);
	if (WIFSIGNALED(wait_status)) {
	    errmsg = vstrallocf(_("%s terminated with signal %d: see %s"),
				cmd, WTERMSIG(wait_status), dbfn());
	} else if (WIFEXITED(wait_status)) {
	    if (WEXITSTATUS(wait_status) != 0) {
		errmsg = vstrallocf(_("%s exited with status %d: see %s"),
				    cmd, WEXITSTATUS(wait_status), dbfn());
	    } else {
		/* Normal exit */
	    }
	} else {
	    errmsg = vstrallocf(_("%s got bad exit: see %s"), cmd, dbfn());
	}
	dbprintf(_("after %s %s wait\n"), cmd, qdisk);

	g_ptr_array_free_full(argv_ptr);

	aclose(nullfd);
	afclose(dumpout);

	fprintf(stdout, "%d %lld 1\n", level, (long long)size);
    }
    amfree(qdisk);
    amfree(cmd);
    return;

common_error:
    dbprintf("%s\n", errmsg);
    qerrmsg = quote_string(errmsg);
    amfree(qdisk);
    dbprintf("%s", errmsg);
    fprintf(stdout, "ERROR %s\n", qerrmsg);
    amfree(errmsg);
    amfree(qerrmsg);
    amfree(cmd);
}