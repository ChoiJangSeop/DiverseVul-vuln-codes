entry *load_entry(FILE * file, void (*error_func) (), struct passwd *pw,
	char **envp) {
	/* this function reads one crontab entry -- the next -- from a file.
	 * it skips any leading blank lines, ignores comments, and returns
	 * NULL if for any reason the entry can't be read and parsed.
	 *
	 * the entry is also parsed here.
	 *
	 * syntax:
	 *   user crontab:
	 *  minutes hours doms months dows cmd\n
	 *   system crontab (/etc/crontab):
	 *  minutes hours doms months dows USERNAME cmd\n
	 */

	ecode_e ecode = e_none;
	entry *e;
	int ch;
	char cmd[MAX_COMMAND];
	char envstr[MAX_ENVSTR];
	char **tenvp;
	char *p;
	struct passwd temppw;

	Debug(DPARS, ("load_entry()...about to eat comments\n"));

	skip_comments(file);

	ch = get_char(file);
	if (ch == EOF)
		return (NULL);

	/* ch is now the first useful character of a useful line.
	 * it may be an @special or it may be the first character
	 * of a list of minutes.
	 */

	e = (entry *) calloc(sizeof (entry), sizeof (char));

	/* check for '-' as a first character, this option will disable 
	* writing a syslog message about command getting executed
	*/
	if (ch == '-') {
	/* if we are editing system crontab or user uid is 0 (root) 
	* we are allowed to disable logging 
	*/
		if (pw == NULL || pw->pw_uid == 0)
			e->flags |= DONT_LOG;
		else {
			log_it("CRON", getpid(), "ERROR", "Only privileged user can disable logging", 0);
			ecode = e_option;
			goto eof;
		}
		ch = get_char(file);
		if (ch == EOF) {
			free(e);
			return NULL;
		}
	}

	if (ch == '@') {
		/* all of these should be flagged and load-limited; i.e.,
		 * instead of @hourly meaning "0 * * * *" it should mean
		 * "close to the front of every hour but not 'til the
		 * system load is low".  Problems are: how do you know
		 * what "low" means? (save me from /etc/cron.conf!) and:
		 * how to guarantee low variance (how low is low?), which
		 * means how to we run roughly every hour -- seems like
		 * we need to keep a history or let the first hour set
		 * the schedule, which means we aren't load-limited
		 * anymore.  too much for my overloaded brain. (vix, jan90)
		 * HINT
		 */
		ch = get_string(cmd, MAX_COMMAND, file, " \t\n");
		if (!strcmp("reboot", cmd)) {
			e->flags |= WHEN_REBOOT;
		}
		else if (!strcmp("yearly", cmd) || !strcmp("annually", cmd)) {
			bit_set(e->minute, 0);
			bit_set(e->hour, 0);
			bit_set(e->dom, 0);
			bit_set(e->month, 0);
			bit_nset(e->dow, 0, LAST_DOW - FIRST_DOW);
			e->flags |= DOW_STAR;
		}
		else if (!strcmp("monthly", cmd)) {
			bit_set(e->minute, 0);
			bit_set(e->hour, 0);
			bit_set(e->dom, 0);
			bit_nset(e->month, 0, LAST_MONTH - FIRST_MONTH);
			bit_nset(e->dow, 0, LAST_DOW - FIRST_DOW);
			e->flags |= DOW_STAR;
		}
		else if (!strcmp("weekly", cmd)) {
			bit_set(e->minute, 0);
			bit_set(e->hour, 0);
			bit_nset(e->dom, 0, LAST_DOM - FIRST_DOM);
			bit_nset(e->month, 0, LAST_MONTH - FIRST_MONTH);
			bit_set(e->dow, 0);
			e->flags |= DOW_STAR;
		}
		else if (!strcmp("daily", cmd) || !strcmp("midnight", cmd)) {
			bit_set(e->minute, 0);
			bit_set(e->hour, 0);
			bit_nset(e->dom, 0, LAST_DOM - FIRST_DOM);
			bit_nset(e->month, 0, LAST_MONTH - FIRST_MONTH);
			bit_nset(e->dow, 0, LAST_DOW - FIRST_DOW);
		}
		else if (!strcmp("hourly", cmd)) {
			bit_set(e->minute, 0);
			bit_nset(e->hour, 0, LAST_HOUR - FIRST_HOUR);
			bit_nset(e->dom, 0, LAST_DOM - FIRST_DOM);
			bit_nset(e->month, 0, LAST_MONTH - FIRST_MONTH);
			bit_nset(e->dow, 0, LAST_DOW - FIRST_DOW);
			e->flags |= HR_STAR;
		}
		else {
			ecode = e_timespec;
			goto eof;
		}
		/* Advance past whitespace between shortcut and
		 * username/command.
		 */
		Skip_Blanks(ch, file);
		if (ch == EOF || ch == '\n') {
			ecode = e_cmd;
			goto eof;
		}
	}
	else {
		Debug(DPARS, ("load_entry()...about to parse numerics\n"));

		if (ch == '*')
			e->flags |= MIN_STAR;
		ch = get_list(e->minute, FIRST_MINUTE, LAST_MINUTE, PPC_NULL, ch, file);
		if (ch == EOF) {
			ecode = e_minute;
			goto eof;
		}

		/* hours
		 */

		if (ch == '*')
			e->flags |= HR_STAR;
		ch = get_list(e->hour, FIRST_HOUR, LAST_HOUR, PPC_NULL, ch, file);
		if (ch == EOF) {
			ecode = e_hour;
			goto eof;
		}

		/* DOM (days of month)
		 */

		if (ch == '*')
			e->flags |= DOM_STAR;
		ch = get_list(e->dom, FIRST_DOM, LAST_DOM, PPC_NULL, ch, file);
		if (ch == EOF) {
			ecode = e_dom;
			goto eof;
		}

		/* month
		 */

		ch = get_list(e->month, FIRST_MONTH, LAST_MONTH, MonthNames, ch, file);
		if (ch == EOF) {
			ecode = e_month;
			goto eof;
		}

		/* DOW (days of week)
		 */

		if (ch == '*')
			e->flags |= DOW_STAR;
		ch = get_list(e->dow, FIRST_DOW, LAST_DOW, DowNames, ch, file);
		if (ch == EOF) {
			ecode = e_dow;
			goto eof;
		}
	}

	/* make sundays equivalent */
	if (bit_test(e->dow, 0) || bit_test(e->dow, 7)) {
		bit_set(e->dow, 0);
		bit_set(e->dow, 7);
	}

	/* check for permature EOL and catch a common typo */
	if (ch == '\n' || ch == '*') {
		ecode = e_cmd;
		goto eof;
	}

	/* ch is the first character of a command, or a username */
	unget_char(ch, file);

	if (!pw) {
		char *username = cmd;	/* temp buffer */

		Debug(DPARS, ("load_entry()...about to parse username\n"));
		ch = get_string(username, MAX_COMMAND, file, " \t\n");

		Debug(DPARS, ("load_entry()...got %s\n", username));
		if (ch == EOF || ch == '\n' || ch == '*') {
			ecode = e_cmd;
			goto eof;
		}

		pw = getpwnam(username);
		if (pw == NULL) {
			Debug(DPARS, ("load_entry()...unknown user entry\n"));
			memset(&temppw, 0, sizeof (temppw));
			temppw.pw_name = username;
			temppw.pw_passwd = "";
			pw = &temppw;
		} else {
			Debug(DPARS, ("load_entry()...uid %ld, gid %ld\n",
				(long) pw->pw_uid, (long) pw->pw_gid));
		}
	}

	if ((e->pwd = pw_dup(pw)) == NULL) {
		ecode = e_memory;
		goto eof;
	}
	memset(e->pwd->pw_passwd, 0, strlen(e->pwd->pw_passwd));

	p = env_get("RANDOM_DELAY", envp);
	if (p) {
		char *endptr;
		long val;

		errno = 0;    /* To distinguish success/failure after call */
		val = strtol(p, &endptr, 10);
		if (errno != 0 || val < 0 || val > 24*60) {
			log_it("CRON", getpid(), "ERROR", "bad value of RANDOM_DELAY", 0);
		} else {
			e->delay = (int)((double)val * RandomScale);
		}
	}

	/* copy and fix up environment.  some variables are just defaults and
	 * others are overrides.
	 */
	if ((e->envp = env_copy(envp)) == NULL) {
		ecode = e_memory;
		goto eof;
	}
	if (!env_get("SHELL", e->envp)) {
		if (glue_strings(envstr, sizeof envstr, "SHELL", _PATH_BSHELL, '=')) {
			if ((tenvp = env_set(e->envp, envstr)) == NULL) {
				ecode = e_memory;
				goto eof;
			}
			e->envp = tenvp;
		}
		else
			log_it("CRON", getpid(), "ERROR", "can't set SHELL", 0);
	}
	if ((tenvp = env_update_home(e->envp, pw->pw_dir)) == NULL) {
		ecode = e_memory;
		goto eof;
	}
	e->envp = tenvp;
#ifndef LOGIN_CAP
	/* If login.conf is in used we will get the default PATH later. */
	if (!env_get("PATH", e->envp)) {
		char *defpath;

		if (ChangePath)
			defpath = _PATH_DEFPATH;
		else {
			defpath = getenv("PATH");
			if (defpath == NULL)
				defpath = _PATH_DEFPATH;
		}

		if (glue_strings(envstr, sizeof envstr, "PATH", defpath, '=')) {
			if ((tenvp = env_set(e->envp, envstr)) == NULL) {
				ecode = e_memory;
				goto eof;
			}
			e->envp = tenvp;
		}
		else
			log_it("CRON", getpid(), "ERROR", "can't set PATH", 0);
	}
#endif /* LOGIN_CAP */
	if (glue_strings(envstr, sizeof envstr, "LOGNAME", pw->pw_name, '=')) {
		if ((tenvp = env_set(e->envp, envstr)) == NULL) {
			ecode = e_memory;
			goto eof;
		}
		e->envp = tenvp;
	}
	else
		log_it("CRON", getpid(), "ERROR", "can't set LOGNAME", 0);
#if defined(BSD) || defined(__linux)
	if (glue_strings(envstr, sizeof envstr, "USER", pw->pw_name, '=')) {
		if ((tenvp = env_set(e->envp, envstr)) == NULL) {
			ecode = e_memory;
			goto eof;
		}
		e->envp = tenvp;
	}
	else
		log_it("CRON", getpid(), "ERROR", "can't set USER", 0);
#endif

	Debug(DPARS, ("load_entry()...about to parse command\n"));

	/* Everything up to the next \n or EOF is part of the command...
	 * too bad we don't know in advance how long it will be, since we
	 * need to malloc a string for it... so, we limit it to MAX_COMMAND.
	 */
	ch = get_string(cmd, MAX_COMMAND, file, "\n");

	/* a file without a \n before the EOF is rude, so we'll complain...
	 */
	if (ch == EOF) {
		ecode = e_cmd;
		goto eof;
	}

	/* got the command in the 'cmd' string; save it in *e.
	 */
	if ((e->cmd = strdup(cmd)) == NULL) {
		ecode = e_memory;
		goto eof;
	}

	Debug(DPARS, ("load_entry()...returning successfully\n"));

		/* success, fini, return pointer to the entry we just created...
		 */
		return (e);

  eof:
	if (e->envp)
		env_free(e->envp);
	free(e->pwd);
	free(e->cmd);
	free(e);
	while (ch != '\n' && !feof(file))
		ch = get_char(file);
	if (ecode != e_none && error_func)
		(*error_func) (ecodes[(int) ecode]);
	return (NULL);
}