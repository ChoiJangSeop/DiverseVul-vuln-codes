main(int argc, char **argv)
{
	const char *safepath = "/bin:/sbin:/usr/bin:/usr/sbin:"
	    "/usr/local/bin:/usr/local/sbin";
	const char *confpath = NULL;
	char *shargv[] = { NULL, NULL };
	char *sh;
	const char *cmd;
	char cmdline[LINE_MAX];
#ifdef __OpenBSD__
	char mypwbuf[_PW_BUF_LEN], targpwbuf[_PW_BUF_LEN];
#else
	char *mypwbuf = NULL, *targpwbuf = NULL;
#endif
	struct passwd mypwstore, targpwstore;
	struct passwd *mypw, *targpw;
	const struct rule *rule;
	uid_t uid;
	uid_t target = 0;
	gid_t groups[NGROUPS_MAX + 1];
	int ngroups;
	int i, ch, rv;
	int sflag = 0;
	int nflag = 0;
	char cwdpath[PATH_MAX];
	const char *cwd;
	char **envp;
#ifdef USE_BSD_AUTH
	char *login_style = NULL;
#endif

	setprogname("doas");

	closefrom(STDERR_FILENO + 1);

	uid = getuid();

#ifdef USE_BSD_AUTH
# define OPTSTRING "a:C:Lnsu:"
#else
# define OPTSTRING "+C:Lnsu:"
#endif

	while ((ch = getopt(argc, argv, OPTSTRING)) != -1) {
		switch (ch) {
#ifdef USE_BSD_AUTH
		case 'a':
			login_style = optarg;
			break;
#endif
		case 'C':
			confpath = optarg;
			break;
		case 'L':
#if defined(USE_BSD_AUTH)
			i = open("/dev/tty", O_RDWR);
			if (i != -1)
				ioctl(i, TIOCCLRVERAUTH);
			exit(i == -1);
#elif defined(USE_TIMESTAMP)
			exit(timestamp_clear() == -1);
#else
			exit(0);
#endif
		case 'u':
			if (parseuid(optarg, &target) != 0)
				errx(1, "unknown user");
			break;
		case 'n':
			nflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		default:
			usage();
			break;
		}
	}
	argv += optind;
	argc -= optind;

	if (confpath) {
		if (sflag)
			usage();
	} else if ((!sflag && !argc) || (sflag && argc))
		usage();

#ifdef __OpenBSD__
	rv = getpwuid_r(uid, &mypwstore, mypwbuf, sizeof(mypwbuf), &mypw);
	if (rv != 0)
		err(1, "getpwuid_r failed");
#else
	for (size_t sz = 1024; sz <= 16*1024; sz *= 2) {
		mypwbuf = reallocarray(mypwbuf, sz, sizeof (char));
		if (mypwbuf == NULL)
			errx(1, "can't allocate mypwbuf");
		rv = getpwuid_r(uid, &mypwstore, mypwbuf, sz, &mypw);
		if (rv != ERANGE)
			break;
	}
	if (rv != 0)
		err(1, "getpwuid_r failed");
#endif
	if (mypw == NULL)
		errx(1, "no passwd entry for self");
	ngroups = getgroups(NGROUPS_MAX, groups);
	if (ngroups == -1)
		err(1, "can't get groups");
	groups[ngroups++] = getgid();

	if (sflag) {
		sh = getenv("SHELL");
		if (sh == NULL || *sh == '\0') {
			shargv[0] = mypw->pw_shell;
		} else
			shargv[0] = sh;
		argv = shargv;
		argc = 1;
	}

	if (confpath) {
		checkconfig(confpath, argc, argv, uid, groups, ngroups,
		    target);
		exit(1);	/* fail safe */
	}

	if (geteuid())
		errx(1, "not installed setuid");

	parseconfig("/etc/doas.conf", 1);

	/* cmdline is used only for logging, no need to abort on truncate */
	(void)strlcpy(cmdline, argv[0], sizeof(cmdline));
	for (i = 1; i < argc; i++) {
		if (strlcat(cmdline, " ", sizeof(cmdline)) >= sizeof(cmdline))
			break;
		if (strlcat(cmdline, argv[i], sizeof(cmdline)) >= sizeof(cmdline))
			break;
	}

	cmd = argv[0];
	if (!permit(uid, groups, ngroups, &rule, target, cmd,
	    (const char **)argv + 1)) {
		syslog(LOG_AUTHPRIV | LOG_NOTICE,
		    "failed command for %s: %s", mypw->pw_name, cmdline);
		errc(1, EPERM, NULL);
	}

#if defined(__OpenBSD__) || defined(USE_SHADOW)
	if (!(rule->options & NOPASS)) {
		if (nflag)
			errx(1, "Authorization required");

# ifdef __OpenBSD__
		authuser(mypw->pw_name, login_style, rule->options & PERSIST);
# else
		shadowauth(mypw->pw_name, rule->options & PERSIST);
# endif
	}

# ifdef __OpenBSD__
	if (pledge("stdio rpath getpw exec id", NULL) == -1)
		err(1, "pledge");
# endif

#elif !defined(USE_PAM)
	(void) nflag;
	if (!(rule->options & NOPASS)) {
		errx(1, "Authorization required");
	}
#endif /* !(__OpenBSD__ || USE_SHADOW) && !USE_PAM */

#ifdef __OpenBSD__
	rv = getpwuid_r(target, &targpwstore, targpwbuf, sizeof(targpwbuf), &targpw);
	if (rv != 0)
		errx(1, "no passwd entry for target");
#else
	for (size_t sz = 1024; sz <= 16*1024; sz *= 2) {
		targpwbuf = reallocarray(targpwbuf, sz, sizeof (char));
		if (targpwbuf == NULL)
			errx(1, "can't allocate targpwbuf");
		rv = getpwuid_r(target, &targpwstore, targpwbuf, sz, &targpw);
		if (rv != ERANGE)
			break;
	}
	if (rv != 0)
		err(1, "getpwuid_r failed");
#endif
	if (targpw == NULL)
		err(1, "getpwuid_r failed");

#if defined(USE_PAM)
	pamauth(targpw->pw_name, mypw->pw_name, !nflag, rule->options & NOPASS,
	    rule->options & PERSIST);
#endif

#ifdef HAVE_SETUSERCONTEXT
	if (setusercontext(NULL, targpw, target, LOGIN_SETGROUP |
	    LOGIN_SETPRIORITY | LOGIN_SETRESOURCES | LOGIN_SETUMASK |
	    LOGIN_SETUSER) != 0)
		errx(1, "failed to set user context for target");
#else
	if (setresgid(targpw->pw_gid, targpw->pw_gid, targpw->pw_gid) != 0)
		err(1, "setresgid");
	if (initgroups(targpw->pw_name, targpw->pw_gid) != 0)
		err(1, "initgroups");
	if (setresuid(target, target, target) != 0)
		err(1, "setresuid");
#endif

#ifdef __OpenBSD__
	if (pledge("stdio rpath exec", NULL) == -1)
		err(1, "pledge");
#endif

	if (getcwd(cwdpath, sizeof(cwdpath)) == NULL)
		cwd = "(failed)";
	else
		cwd = cwdpath;

#ifdef __OpenBSD__
	if (pledge("stdio exec", NULL) == -1)
		err(1, "pledge");
#endif

	syslog(LOG_AUTHPRIV | LOG_INFO, "%s ran command %s as %s from %s",
	    mypw->pw_name, cmdline, targpw->pw_name, cwd);

	envp = prepenv(rule);

	if (rule->cmd) {
		if (setenv("PATH", safepath, 1) == -1)
			err(1, "failed to set PATH '%s'", safepath);
	}
	execvpe(cmd, argv, envp);
	if (errno == ENOENT)
		errx(1, "%s: command not found", cmd);
	err(1, "%s", cmd);
}