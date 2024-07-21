main (int argc, char *argv[])
{
    int i = 0;
    time_t t = 0;
    struct sigaction sa;
    unsigned long cachesize = 0;
    char *x = NULL, char_seed[128];

    sa.sa_handler = handle_term;
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGTERM, &sa, NULL);

    sa.sa_handler = SIG_IGN;
    sigaction (SIGPIPE, &sa, NULL);

    seed_addtime ();
    seed_adduint32 (getpid ());
    seed_adduint32 (getppid ());
    seed_adduint32 (getuid ());
    seed_adduint32 (getgid ());

    seed_addtime ();
    prog = strdup ((x = strrchr (argv[0], '/')) != NULL ?  x + 1 : argv[0]);
    i = check_option (argc, argv);
    argc -= i;
    argv += i;

    if (mode & DAEMON)
    {
        i = fork ();
        if (i == -1)
            err (-1, "could not fork a daemon process");
        if (i > 0)
            return 0;
    }

    time (&t);
    memset (char_seed, 0, sizeof (char_seed));
    strftime (char_seed, sizeof (char_seed), "%b-%d %Y %T", localtime (&t));
    fprintf (stderr, "\n");
    warnx ("version %s: starting: %s\n", VERSION, char_seed);

    read_conf (CFGFILE);

    if ((x = env_get ("DATALIMIT")))
    {
        struct rlimit r;
        unsigned long dlimit = atol (x);

        if (getrlimit (RLIMIT_DATA,  &r) != 0)
            err (-1, "could not get resource RLIMIT_DATA");

        r.rlim_cur = (dlimit <= r.rlim_max) ? dlimit : r.rlim_max;

        if (setrlimit (RLIMIT_DATA, &r) != 0)
            err (-1, "could not set resource RLIMIT_DATA");

        if (debug_level)
            warnx ("DATALIMIT set to `%ld' bytes", r.rlim_cur);
    }

    if (!(x = env_get ("IP")))
        err (-1, "$IP not set");
    if (!ip4_scan (x, myipincoming))
        err (-1, "could not parse IP address `%s'", x);

    seed_addtime ();
    udp53 = socket_udp ();
    if (udp53 == -1)
        err (-1, "could not open UDP socket");
    if (socket_bind4_reuse (udp53, myipincoming, 53) == -1)
        err (-1, "could not bind UDP socket");

    seed_addtime ();
    tcp53 = socket_tcp ();
    if (tcp53 == -1)
        err (-1, "could not open TCP socket");
    if (socket_bind4_reuse (tcp53, myipincoming, 53) == -1)
        err (-1, "could not bind TCP socket");

    if (mode & DAEMON)
    {
        /* redirect stdout & stderr to a log file */
        redirect_to_log (LOGFILE);

        write_pid (PIDFILE);
    }

    seed_addtime ();
    droproot ();
    if (mode & DAEMON)
        /* crerate a new session & detach from controlling tty */
        if (setsid () < 0)
            err (-1, "could not start a new session for the daemon");

    seed_addtime ();
    socket_tryreservein (udp53, 131072);

    memset (char_seed, 0, sizeof (char_seed));
    for (i = 0, x = (char *)seed; i < sizeof (char_seed); i++, x++)
        char_seed[i] = *x;
    dns_random_init (char_seed);

    if (!(x = env_get ("IPSEND")))
        err (-1, "$IPSEND not set");
    if (!ip4_scan (x, myipoutgoing))
        err (-1, "could not parse IP address `%s'", x);

    if (!(x = env_get ("CACHESIZE")))
        err (-1, "$CACHESIZE not set");
    scan_ulong (x, &cachesize);
    if (!cache_init (cachesize))
        err (-1, "could not allocate `%ld' bytes for cache", cachesize);

    if (env_get ("HIDETTL"))
        response_hidettl ();
    if (env_get ("FORWARDONLY"))
        query_forwardonly ();
    if (!roots_init ())
        err (-1, "could not read servers");
    if (socket_listen (tcp53, 20) == -1)
        err (-1, "could not listen on TCP socket");

    doit ();

    return 0;
}