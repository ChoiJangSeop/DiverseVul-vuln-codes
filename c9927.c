sudoers_policy_main(int argc, char * const argv[], int pwflag, char *env_add[],
    bool verbose, void *closure)
{
    char *iolog_path = NULL;
    mode_t cmnd_umask = ACCESSPERMS;
    struct sudo_nss *nss;
    int oldlocale, validated, ret = -1;
    debug_decl(sudoers_policy_main, SUDOERS_DEBUG_PLUGIN);

    sudo_warn_set_locale_func(sudoers_warn_setlocale);

    unlimit_nproc();

    /* Is root even allowed to run sudo? */
    if (user_uid == 0 && !def_root_sudo) {
	/* Not an audit event (should it be?). */
	sudo_warnx("%s",
	    U_("sudoers specifies that root is not allowed to sudo"));
	goto bad;
    }

    if (!set_perms(PERM_INITIAL))
	goto bad;

    /* Environment variables specified on the command line. */
    if (env_add != NULL && env_add[0] != NULL)
	sudo_user.env_vars = env_add;

    /*
     * Make a local copy of argc/argv, with special handling
     * for pseudo-commands and the '-i' option.
     */
    if (argc == 0) {
	NewArgc = 1;
	NewArgv = reallocarray(NULL, NewArgc + 1, sizeof(char *));
	if (NewArgv == NULL) {
	    sudo_warnx(U_("%s: %s"), __func__, U_("unable to allocate memory"));
	    goto done;
	}
	sudoers_gc_add(GC_VECTOR, NewArgv);
	NewArgv[0] = user_cmnd;
	NewArgv[1] = NULL;
    } else {
	/* Must leave an extra slot before NewArgv for bash's --login */
	NewArgc = argc;
	NewArgv = reallocarray(NULL, NewArgc + 2, sizeof(char *));
	if (NewArgv == NULL) {
	    sudo_warnx(U_("%s: %s"), __func__, U_("unable to allocate memory"));
	    goto done;
	}
	sudoers_gc_add(GC_VECTOR, NewArgv);
	NewArgv++;	/* reserve an extra slot for --login */
	memcpy(NewArgv, argv, argc * sizeof(char *));
	NewArgv[NewArgc] = NULL;
	if (ISSET(sudo_mode, MODE_LOGIN_SHELL) && runas_pw != NULL) {
	    NewArgv[0] = strdup(runas_pw->pw_shell);
	    if (NewArgv[0] == NULL) {
		sudo_warnx(U_("%s: %s"), __func__, U_("unable to allocate memory"));
		goto done;
	    }
	    sudoers_gc_add(GC_PTR, NewArgv[0]);
	}
    }

    /* If given the -P option, set the "preserve_groups" flag. */
    if (ISSET(sudo_mode, MODE_PRESERVE_GROUPS))
	def_preserve_groups = true;

    /* Find command in path and apply per-command Defaults. */
    cmnd_status = set_cmnd();
    if (cmnd_status == NOT_FOUND_ERROR)
	goto done;

    /* Check for -C overriding def_closefrom. */
    if (user_closefrom >= 0 && user_closefrom != def_closefrom) {
	if (!def_closefrom_override) {
	    log_warningx(SLOG_NO_STDERR|SLOG_AUDIT,
		N_("user not allowed to override closefrom limit"));
	    sudo_warnx("%s", U_("you are not permitted to use the -C option"));
	    goto bad;
	}
	def_closefrom = user_closefrom;
    }

    /*
     * Check sudoers sources, using the locale specified in sudoers.
     */
    sudoers_setlocale(SUDOERS_LOCALE_SUDOERS, &oldlocale);
    validated = sudoers_lookup(snl, sudo_user.pw, &cmnd_status, pwflag);
    if (ISSET(validated, VALIDATE_ERROR)) {
	/* The lookup function should have printed an error. */
	goto done;
    }

    /* Restore user's locale. */
    sudoers_setlocale(oldlocale, NULL);

    if (safe_cmnd == NULL) {
	if ((safe_cmnd = strdup(user_cmnd)) == NULL) {
	    sudo_warnx(U_("%s: %s"), __func__, U_("unable to allocate memory"));
	    goto done;
	}
    }

    /* Defer uid/gid checks until after defaults have been updated. */
    if (unknown_runas_uid && !def_runas_allow_unknown_id) {
	log_warningx(SLOG_AUDIT, N_("unknown user: %s"), runas_pw->pw_name);
	goto done;
    }
    if (runas_gr != NULL) {
	if (unknown_runas_gid && !def_runas_allow_unknown_id) {
	    log_warningx(SLOG_AUDIT, N_("unknown group: %s"),
		runas_gr->gr_name);
	    goto done;
	}
    }

    /*
     * Look up the timestamp dir owner if one is specified.
     */
    if (def_timestampowner) {
	struct passwd *pw = NULL;

	if (*def_timestampowner == '#') {
	    const char *errstr;
	    uid_t uid = sudo_strtoid(def_timestampowner + 1, &errstr);
	    if (errstr == NULL)
		pw = sudo_getpwuid(uid);
	}
	if (pw == NULL)
	    pw = sudo_getpwnam(def_timestampowner);
	if (pw != NULL) {
	    timestamp_uid = pw->pw_uid;
	    timestamp_gid = pw->pw_gid;
	    sudo_pw_delref(pw);
	} else {
	    /* XXX - audit too? */
	    log_warningx(SLOG_SEND_MAIL,
		N_("timestamp owner (%s): No such user"), def_timestampowner);
	    timestamp_uid = ROOT_UID;
	    timestamp_gid = ROOT_GID;
	}
    }

    /* If no command line args and "shell_noargs" is not set, error out. */
    if (ISSET(sudo_mode, MODE_IMPLIED_SHELL) && !def_shell_noargs) {
	/* Not an audit event. */
	ret = -2; /* usage error */
	goto done;
    }

    /* Bail if a tty is required and we don't have one.  */
    if (def_requiretty && !tty_present()) {
	log_warningx(SLOG_NO_STDERR|SLOG_AUDIT, N_("no tty"));
	sudo_warnx("%s", U_("sorry, you must have a tty to run sudo"));
	goto bad;
    }

    /* Check runas user's shell. */
    if (!check_user_shell(runas_pw)) {
	log_warningx(SLOG_RAW_MSG|SLOG_AUDIT,
	    N_("invalid shell for user %s: %s"),
	    runas_pw->pw_name, runas_pw->pw_shell);
	goto bad;
    }

    /*
     * We don't reset the environment for sudoedit or if the user
     * specified the -E command line flag and they have setenv privs.
     */
    if (ISSET(sudo_mode, MODE_EDIT) ||
	(ISSET(sudo_mode, MODE_PRESERVE_ENV) && def_setenv))
	def_env_reset = false;

    /* Build a new environment that avoids any nasty bits. */
    if (!rebuild_env())
	goto bad;

    /* Require a password if sudoers says so.  */
    switch (check_user(validated, sudo_mode)) {
    case true:
	/* user authenticated successfully. */
	break;
    case false:
	/* Note: log_denial() calls audit for us. */
	if (!ISSET(validated, VALIDATE_SUCCESS)) {
	    /* Only display a denial message if no password was read. */
	    if (!log_denial(validated, def_passwd_tries <= 0))
		goto done;
	}
	goto bad;
    default:
	/* some other error, ret is -1. */
	goto done;
    }

    /* Check whether user_runchroot is permitted (if specified). */
    switch (check_user_runchroot()) {
    case true:
	break;
    case false:
	goto bad;
    default:
	goto done;
    }

    /* Check whether user_runcwd is permitted (if specified). */
    switch (check_user_runcwd()) {
    case true:
	break;
    case false:
	goto bad;
    default:
	goto done;
    }

    /* If run as root with SUDO_USER set, set sudo_user.pw to that user. */
    /* XXX - causes confusion when root is not listed in sudoers */
    if (sudo_mode & (MODE_RUN | MODE_EDIT) && prev_user != NULL) {
	if (user_uid == 0 && strcmp(prev_user, "root") != 0) {
	    struct passwd *pw;

	    if ((pw = sudo_getpwnam(prev_user)) != NULL) {
		    if (sudo_user.pw != NULL)
			sudo_pw_delref(sudo_user.pw);
		    sudo_user.pw = pw;
	    }
	}
    }

    /* If the user was not allowed to run the command we are done. */
    if (!ISSET(validated, VALIDATE_SUCCESS)) {
	/* Note: log_failure() calls audit for us. */
	if (!log_failure(validated, cmnd_status))
	    goto done;
	goto bad;
    }

    /* Create Ubuntu-style dot file to indicate sudo was successful. */
    if (create_admin_success_flag() == -1)
	goto done;

    /* Finally tell the user if the command did not exist. */
    if (cmnd_status == NOT_FOUND_DOT) {
	audit_failure(NewArgv, N_("command in current directory"));
	sudo_warnx(U_("ignoring \"%s\" found in '.'\nUse \"sudo ./%s\" if this is the \"%s\" you wish to run."), user_cmnd, user_cmnd, user_cmnd);
	goto bad;
    } else if (cmnd_status == NOT_FOUND) {
	if (ISSET(sudo_mode, MODE_CHECK)) {
	    audit_failure(NewArgv, N_("%s: command not found"),
		NewArgv[0]);
	    sudo_warnx(U_("%s: command not found"), NewArgv[0]);
	} else {
	    audit_failure(NewArgv, N_("%s: command not found"),
		user_cmnd);
	    sudo_warnx(U_("%s: command not found"), user_cmnd);
	}
	goto bad;
    }

    /* If user specified a timeout make sure sudoers allows it. */
    if (!def_user_command_timeouts && user_timeout > 0) {
	log_warningx(SLOG_NO_STDERR|SLOG_AUDIT,
	    N_("user not allowed to set a command timeout"));
	sudo_warnx("%s",
	    U_("sorry, you are not allowed set a command timeout"));
	goto bad;
    }

    /* If user specified env vars make sure sudoers allows it. */
    if (ISSET(sudo_mode, MODE_RUN) && !def_setenv) {
	if (ISSET(sudo_mode, MODE_PRESERVE_ENV)) {
	    log_warningx(SLOG_NO_STDERR|SLOG_AUDIT,
		N_("user not allowed to preserve the environment"));
	    sudo_warnx("%s",
		U_("sorry, you are not allowed to preserve the environment"));
	    goto bad;
	} else {
	    if (!validate_env_vars(sudo_user.env_vars))
		goto bad;
	}
    }

    if (ISSET(sudo_mode, (MODE_RUN | MODE_EDIT)) && !remote_iologs) {
	if ((def_log_input || def_log_output) && def_iolog_file && def_iolog_dir) {
	    if ((iolog_path = format_iolog_path()) == NULL) {
		if (!def_ignore_iolog_errors)
		    goto done;
		/* Unable to expand I/O log path, disable I/O logging. */
		def_log_input = false;
		def_log_output = false;
	    }
	}
    }

    switch (sudo_mode & MODE_MASK) {
	case MODE_CHECK:
	    ret = display_cmnd(snl, list_pw ? list_pw : sudo_user.pw);
	    break;
	case MODE_LIST:
	    ret = display_privs(snl, list_pw ? list_pw : sudo_user.pw, verbose);
	    break;
	case MODE_VALIDATE:
	case MODE_RUN:
	case MODE_EDIT:
	    /* ret may be overridden by "goto bad" later */
	    ret = true;
	    break;
	default:
	    /* Should not happen. */
	    sudo_warnx("internal error, unexpected sudo mode 0x%x", sudo_mode);
	    goto done;
    }

    /* Cleanup sudoers sources */
    TAILQ_FOREACH(nss, snl, entries) {
	nss->close(nss);
    }
    if (def_group_plugin)
	group_plugin_unload();
    init_parser(NULL, false, false);

    if (ISSET(sudo_mode, (MODE_VALIDATE|MODE_CHECK|MODE_LIST))) {
	/* ret already set appropriately */
	goto done;
    }

    /*
     * Set umask based on sudoers.
     * If user's umask is more restrictive, OR in those bits too
     * unless umask_override is set.
     */
    if (def_umask != ACCESSPERMS) {
	cmnd_umask = def_umask;
	if (!def_umask_override)
	    cmnd_umask |= user_umask;
    }

    if (ISSET(sudo_mode, MODE_LOGIN_SHELL)) {
	char *p;

	/* Convert /bin/sh -> -sh so shell knows it is a login shell */
	if ((p = strrchr(NewArgv[0], '/')) == NULL)
	    p = NewArgv[0];
	*p = '-';
	NewArgv[0] = p;

	/*
	 * Newer versions of bash require the --login option to be used
	 * in conjunction with the -c option even if the shell name starts
	 * with a '-'.  Unfortunately, bash 1.x uses -login, not --login
	 * so this will cause an error for that.
	 */
	if (NewArgc > 1 && strcmp(NewArgv[0], "-bash") == 0 &&
	    strcmp(NewArgv[1], "-c") == 0) {
	    /* Use the extra slot before NewArgv so we can store --login. */
	    NewArgv--;
	    NewArgc++;
	    NewArgv[0] = NewArgv[1];
	    NewArgv[1] = "--login";
	}

#if defined(_AIX) || (defined(__linux__) && !defined(HAVE_PAM))
	/* Insert system-wide environment variables. */
	if (!read_env_file(_PATH_ENVIRONMENT, true, false))
	    sudo_warn("%s", _PATH_ENVIRONMENT);
#endif
#ifdef HAVE_LOGIN_CAP_H
	/* Set environment based on login class. */
	if (login_class) {
	    login_cap_t *lc = login_getclass(login_class);
	    if (lc != NULL) {
		setusercontext(lc, runas_pw, runas_pw->pw_uid, LOGIN_SETPATH|LOGIN_SETENV);
		login_close(lc);
	    }
	}
#endif /* HAVE_LOGIN_CAP_H */
    }

    /* Insert system-wide environment variables. */
    if (def_restricted_env_file) {
	if (!read_env_file(def_restricted_env_file, false, true))
	    sudo_warn("%s", def_restricted_env_file);
    }
    if (def_env_file) {
	if (!read_env_file(def_env_file, false, false))
	    sudo_warn("%s", def_env_file);
    }

    /* Insert user-specified environment variables. */
    if (!insert_env_vars(sudo_user.env_vars))
	goto done;

    /* Note: must call audit before uid change. */
    if (ISSET(sudo_mode, MODE_EDIT)) {
	char **edit_argv;
	int edit_argc;
	const char *env_editor;

	free(safe_cmnd);
	safe_cmnd = find_editor(NewArgc - 1, NewArgv + 1, &edit_argc,
	    &edit_argv, NULL, &env_editor, false);
	if (safe_cmnd == NULL) {
	    if (errno != ENOENT)
		goto done;
	    audit_failure(NewArgv, N_("%s: command not found"),
		env_editor ? env_editor : def_editor);
	    sudo_warnx(U_("%s: command not found"),
		env_editor ? env_editor : def_editor);
	    goto bad;
	}
	sudoers_gc_add(GC_VECTOR, edit_argv);
	NewArgv = edit_argv;
	NewArgc = edit_argc;

	/* We want to run the editor with the unmodified environment. */
	env_swap_old();
    }

    goto done;

bad:
    ret = false;

done:
    if (ret == -1) {
	/* Free stashed copy of the environment. */
	(void)env_init(NULL);
    } else {
	/* Store settings to pass back to front-end. */
	if (!sudoers_policy_store_result(ret, NewArgv, env_get(), cmnd_umask,
		iolog_path, closure))
	    ret = -1;
    }

    if (!rewind_perms())
	ret = -1;

    restore_nproc();

    /* Destroy the password and group caches and free the contents. */
    sudo_freepwcache();
    sudo_freegrcache();

    sudo_warn_set_locale_func(NULL);

    debug_return_int(ret);
}