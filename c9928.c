set_cmnd(void)
{
    struct sudo_nss *nss;
    int ret = FOUND;
    debug_decl(set_cmnd, SUDOERS_DEBUG_PLUGIN);

    /* Allocate user_stat for find_path() and match functions. */
    user_stat = calloc(1, sizeof(struct stat));
    if (user_stat == NULL) {
	sudo_warnx(U_("%s: %s"), __func__, U_("unable to allocate memory"));
	debug_return_int(NOT_FOUND_ERROR);
    }

    /* Default value for cmnd, overridden below. */
    if (user_cmnd == NULL)
	user_cmnd = NewArgv[0];

    if (sudo_mode & (MODE_RUN | MODE_EDIT | MODE_CHECK)) {
	if (ISSET(sudo_mode, MODE_RUN | MODE_CHECK)) {
	    const char *runchroot = user_runchroot;
	    if (runchroot == NULL && def_runchroot != NULL &&
		    strcmp(def_runchroot, "*") != 0)
		runchroot = def_runchroot;

	    ret = set_cmnd_path(runchroot);
	    if (ret == NOT_FOUND_ERROR) {
		if (errno == ENAMETOOLONG) {
		    audit_failure(NewArgv, N_("command too long"));
		}
		log_warning(0, "%s", NewArgv[0]);
		debug_return_int(ret);
	    }
	}

	/* set user_args */
	if (NewArgc > 1) {
	    char *to, *from, **av;
	    size_t size, n;

	    /* Alloc and build up user_args. */
	    for (size = 0, av = NewArgv + 1; *av; av++)
		size += strlen(*av) + 1;
	    if (size == 0 || (user_args = malloc(size)) == NULL) {
		sudo_warnx(U_("%s: %s"), __func__, U_("unable to allocate memory"));
		debug_return_int(NOT_FOUND_ERROR);
	    }
	    if (ISSET(sudo_mode, MODE_SHELL|MODE_LOGIN_SHELL)) {
		/*
		 * When running a command via a shell, the sudo front-end
		 * escapes potential meta chars.  We unescape non-spaces
		 * for sudoers matching and logging purposes.
		 */
		for (to = user_args, av = NewArgv + 1; (from = *av); av++) {
		    while (*from) {
			if (from[0] == '\\' && !isspace((unsigned char)from[1]))
			    from++;
			*to++ = *from++;
		    }
		    *to++ = ' ';
		}
		*--to = '\0';
	    } else {
		for (to = user_args, av = NewArgv + 1; *av; av++) {
		    n = strlcpy(to, *av, size - (to - user_args));
		    if (n >= size - (to - user_args)) {
			sudo_warnx(U_("internal error, %s overflow"), __func__);
			debug_return_int(NOT_FOUND_ERROR);
		    }
		    to += n;
		    *to++ = ' ';
		}
		*--to = '\0';
	    }
	}
    }

    if ((user_base = strrchr(user_cmnd, '/')) != NULL)
	user_base++;
    else
	user_base = user_cmnd;

    /* Convert "sudo sudoedit" -> "sudoedit" */
    if (ISSET(sudo_mode, MODE_RUN) && strcmp(user_base, "sudoedit") == 0) {
	CLR(sudo_mode, MODE_RUN);
	SET(sudo_mode, MODE_EDIT);
	sudo_warnx("%s", U_("sudoedit doesn't need to be run via sudo"));
	user_base = user_cmnd = "sudoedit";
    }

    TAILQ_FOREACH(nss, snl, entries) {
	if (!update_defaults(nss->parse_tree, NULL, SETDEF_CMND, false)) {
	    log_warningx(SLOG_SEND_MAIL|SLOG_NO_STDERR,
		N_("problem with defaults entries"));
	}
    }

    debug_return_int(ret);
}