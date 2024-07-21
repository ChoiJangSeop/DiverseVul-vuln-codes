ambsdtar_backup(
    application_argument_t *argument)
{
    int         dumpin;
    char      *cmd = NULL;
    char      *qdisk;
    char      *timestamps;
    int        mesgf = 3;
    int        indexf = 4;
    int        outf;
    int        data_out;
    int        index_out;
    int        index_err;
    char      *errmsg = NULL;
    amwait_t   wait_status;
    GPtrArray *argv_ptr;
    pid_t      tarpid;
    pid_t      indexpid = 0;
    time_t     tt;
    char      *file_exclude;
    char      *file_include;
    char       new_timestamps[64];

    mesgstream = fdopen(mesgf, "w");
    if (!mesgstream) {
	error(_("error mesgstream(%d): %s\n"), mesgf, strerror(errno));
    }

    if (!bsdtar_path) {
	error(_("BSDTAR-PATH not defined"));
    }

    if (!check_exec_for_suid(bsdtar_path, FALSE)) {
	error("'%s' binary is not secure", bsdtar_path);
    }

    if (!state_dir) {
	error(_("STATE-DIR not defined"));
    }

    if (!argument->level) {
        fprintf(mesgstream, "? No level argument\n");
        error(_("No level argument"));
    }
    if (!argument->dle.disk) {
        fprintf(mesgstream, "? No disk argument\n");
        error(_("No disk argument"));
    }
    if (!argument->dle.device) {
        fprintf(mesgstream, "? No device argument\n");
        error(_("No device argument"));
    }

    qdisk = quote_string(argument->dle.disk);

    tt = time(NULL);
    ctime_r(&tt, new_timestamps);
    new_timestamps[strlen(new_timestamps)-1] = '\0';
    timestamps = ambsdtar_get_timestamps(argument,
				   GPOINTER_TO_INT(argument->level->data),
				   mesgstream, CMD_BACKUP);
    cmd = g_strdup(bsdtar_path);
    argv_ptr = ambsdtar_build_argv(argument, timestamps, &file_exclude,
				   &file_include, CMD_BACKUP);

    if (argument->dle.create_index) {
	tarpid = pipespawnv(cmd, STDIN_PIPE|STDOUT_PIPE|STDERR_PIPE, 1,
			&dumpin, &data_out, &outf, (char **)argv_ptr->pdata);
	g_ptr_array_free_full(argv_ptr);
	argv_ptr = g_ptr_array_new();
	g_ptr_array_add(argv_ptr, g_strdup(bsdtar_path));
	g_ptr_array_add(argv_ptr, g_strdup("tf"));
	g_ptr_array_add(argv_ptr, g_strdup("-"));
	g_ptr_array_add(argv_ptr, NULL);
	indexpid = pipespawnv(cmd, STDIN_PIPE|STDOUT_PIPE|STDERR_PIPE, 1,
			  &index_in, &index_out, &index_err,
			  (char **)argv_ptr->pdata);
	g_ptr_array_free_full(argv_ptr);

	aclose(dumpin);

	indexstream = fdopen(indexf, "w");
	if (!indexstream) {
	    error(_("error indexstream(%d): %s\n"), indexf, strerror(errno));
	}
	read_fd(data_out , "data out" , &read_data_out);
	read_fd(outf     , "data err" , &read_text);
	read_fd(index_out, "index out", &read_text);
	read_fd(index_err, "index_err", &read_text);
    } else {
	tarpid = pipespawnv(cmd, STDIN_PIPE|STDERR_PIPE, 1,
			&dumpin, &dataf, &outf, (char **)argv_ptr->pdata);
	g_ptr_array_free_full(argv_ptr);
	aclose(dumpin);
	aclose(dataf);
	read_fd(outf, "data err", &read_text);
    }

    event_loop(0);

    waitpid(tarpid, &wait_status, 0);
    if (WIFSIGNALED(wait_status)) {
	errmsg = g_strdup_printf(_("%s terminated with signal %d: see %s"),
			    cmd, WTERMSIG(wait_status), dbfn());
	exit_status = 1;
    } else if (WIFEXITED(wait_status)) {
	if (exit_value[WEXITSTATUS(wait_status)] == 1) {
	    errmsg = g_strdup_printf(_("%s exited with status %d: see %s"),
				cmd, WEXITSTATUS(wait_status), dbfn());
	    exit_status = 1;
	} else {
	    /* Normal exit */
	}
    } else {
	errmsg = g_strdup_printf(_("%s got bad exit: see %s"),
			    cmd, dbfn());
	exit_status = 1;
    }
    if (argument->dle.create_index) {
	waitpid(indexpid, &wait_status, 0);
	if (WIFSIGNALED(wait_status)) {
	    errmsg = g_strdup_printf(_("'%s index' terminated with signal %d: see %s"),
			    cmd, WTERMSIG(wait_status), dbfn());
	    exit_status = 1;
	} else if (WIFEXITED(wait_status)) {
	    if (exit_value[WEXITSTATUS(wait_status)] == 1) {
		errmsg = g_strdup_printf(_("'%s index' exited with status %d: see %s"),
				cmd, WEXITSTATUS(wait_status), dbfn());
		exit_status = 1;
	    } else {
		/* Normal exit */
	    }
	} else {
	    errmsg = g_strdup_printf(_("'%s index' got bad exit: see %s"),
			    cmd, dbfn());
	    exit_status = 1;
	}
    }
    g_debug(_("after %s %s wait"), cmd, qdisk);
    g_debug(_("ambsdtar: %s: pid %ld"), cmd, (long)tarpid);
    if (errmsg) {
	g_debug("%s", errmsg);
	g_fprintf(mesgstream, "sendbackup: error [%s]\n", errmsg);
	exit_status = 1;
    } else if (argument->dle.record) {
	ambsdtar_set_timestamps(argument,
				GPOINTER_TO_INT(argument->level->data),
				new_timestamps,
				mesgstream);
    }


    g_debug("sendbackup: size %lld", (long long)dump_size);
    fprintf(mesgstream, "sendbackup: size %lld\n", (long long)dump_size);

    if (argument->dle.create_index)
	fclose(indexstream);

    fclose(mesgstream);

    if (argument->verbose == 0) {
	if (file_exclude)
	    unlink(file_exclude);
	if (file_include)
	    unlink(file_include);
    }

    amfree(file_exclude);
    amfree(file_include);
    amfree(timestamps);
    amfree(qdisk);
    amfree(cmd);
    amfree(errmsg);
}