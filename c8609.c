tgetpass(const char *prompt, int timeout, int flags,
    struct sudo_conv_callback *callback)
{
    struct sigaction sa, savealrm, saveint, savehup, savequit, saveterm;
    struct sigaction savetstp, savettin, savettou;
    char *pass;
    static const char *askpass;
    static char buf[SUDO_CONV_REPL_MAX + 1];
    int i, input, output, save_errno, ttyfd;
    bool need_restart, neednl = false;
    enum tgetpass_errval errval;
    debug_decl(tgetpass, SUDO_DEBUG_CONV);

    (void) fflush(stdout);

    if (askpass == NULL) {
	askpass = getenv_unhooked("SUDO_ASKPASS");
	if (askpass == NULL || *askpass == '\0')
	    askpass = sudo_conf_askpass_path();
    }

restart:
    /* Try to open /dev/tty if we are going to be using it for I/O. */
    ttyfd = -1;
    if (!ISSET(flags, TGP_STDIN|TGP_ASKPASS)) {
	ttyfd = open(_PATH_TTY, O_RDWR);
	/* If no tty present and we need to disable echo, try askpass. */
	if (ttyfd == -1 && !ISSET(flags, TGP_ECHO|TGP_NOECHO_TRY)) {
	    if (askpass == NULL || getenv_unhooked("DISPLAY") == NULL) {
		sudo_warnx(U_("a terminal is required to read the password; either use the -S option to read from standard input or configure an askpass helper"));
		debug_return_str(NULL);
	    }
	    SET(flags, TGP_ASKPASS);
	}
    }

    /* If using a helper program to get the password, run it instead. */
    if (ISSET(flags, TGP_ASKPASS)) {
	if (askpass == NULL || *askpass == '\0')
	    sudo_fatalx(U_("no askpass program specified, try setting SUDO_ASKPASS"));
	debug_return_str_masked(sudo_askpass(askpass, prompt));
    }

    /* Reset state. */
    for (i = 0; i < NSIG; i++)
	signo[i] = 0;
    pass = NULL;
    save_errno = 0;
    need_restart = false;

    /* Use tty for reading/writing if available else use stdin/stderr. */
    if (ttyfd == -1) {
	input = STDIN_FILENO;
	output = STDERR_FILENO;
    } else {
	input = ttyfd;
	output = ttyfd;
    }

    /*
     * If we are using a tty but are not the foreground pgrp this will
     * return EINTR.  We send ourself SIGTTOU bracketed by callbacks.
     */
    if (!ISSET(flags, TGP_ECHO)) {
	for (;;) {
	    if (ISSET(flags, TGP_MASK))
		neednl = sudo_term_cbreak(input);
	    else
		neednl = sudo_term_noecho(input);
	    if (neednl || errno != EINTR)
		break;
	    /* Received SIGTTOU, suspend the process. */
	    if (suspend(SIGTTOU, callback) == -1) {
		if (input != STDIN_FILENO)
		    (void) close(input);
		debug_return_ptr(NULL);
	    }
	}
    }

    /*
     * Catch signals that would otherwise cause the user to end
     * up with echo turned off in the shell.
     */
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;	/* don't restart system calls */
    sa.sa_handler = tgetpass_handler;
    (void) sigaction(SIGALRM, &sa, &savealrm);
    (void) sigaction(SIGINT, &sa, &saveint);
    (void) sigaction(SIGHUP, &sa, &savehup);
    (void) sigaction(SIGQUIT, &sa, &savequit);
    (void) sigaction(SIGTERM, &sa, &saveterm);
    (void) sigaction(SIGTSTP, &sa, &savetstp);
    (void) sigaction(SIGTTIN, &sa, &savettin);
    (void) sigaction(SIGTTOU, &sa, &savettou);

    if (ISSET(flags, TGP_BELL) && output != STDERR_FILENO) {
	/* Ring the bell if requested and there is a tty. */
	if (write(output, "\a", 1) == -1)
	    goto restore;
    }
    if (prompt) {
	if (write(output, prompt, strlen(prompt)) == -1)
	    goto restore;
    }

    if (timeout > 0)
	alarm(timeout);
    pass = getln(input, buf, sizeof(buf), ISSET(flags, TGP_MASK), &errval);
    alarm(0);
    save_errno = errno;

    if (neednl || pass == NULL) {
	if (write(output, "\n", 1) == -1)
	    goto restore;
    }
    tgetpass_display_error(errval);

restore:
    /* Restore old signal handlers. */
    (void) sigaction(SIGALRM, &savealrm, NULL);
    (void) sigaction(SIGINT, &saveint, NULL);
    (void) sigaction(SIGHUP, &savehup, NULL);
    (void) sigaction(SIGQUIT, &savequit, NULL);
    (void) sigaction(SIGTERM, &saveterm, NULL);
    (void) sigaction(SIGTSTP, &savetstp, NULL);
    (void) sigaction(SIGTTIN, &savettin, NULL);
    (void) sigaction(SIGTTOU, &savettou, NULL);

    /* Restore old tty settings. */
    if (!ISSET(flags, TGP_ECHO)) {
	/* Restore old tty settings if possible. */
	(void) sudo_term_restore(input, true);
    }
    if (input != STDIN_FILENO)
	(void) close(input);

    /*
     * If we were interrupted by a signal, resend it to ourselves
     * now that we have restored the signal handlers.
     */
    for (i = 0; i < NSIG; i++) {
	if (signo[i]) {
	    switch (i) {
		case SIGALRM:
		    break;
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
		    if (suspend(i, callback) == 0)
			need_restart = true;
		    break;
		default:
		    kill(getpid(), i);
		    break;
	    }
	}
    }
    if (need_restart)
	goto restart;

    if (save_errno)
	errno = save_errno;

    debug_return_str_masked(pass);
}