process_options(argc, argv)
int argc;
char *argv[];
{
    int i, l;

    /*
     * Process options.
     */
    while (argc > 1 && argv[1][0] == '-') {
        argv++;
        argc--;
        l = (int) strlen(*argv);
        /* must supply at least 4 chars to match "-XXXgraphics" */
        if (l < 4)
            l = 4;

        switch (argv[0][1]) {
        case 'D':
        case 'd':
            if ((argv[0][1] == 'D' && !argv[0][2])
                || !strcmpi(*argv, "-debug")) {
                wizard = TRUE, discover = FALSE;
            } else if (!strncmpi(*argv, "-DECgraphics", l)) {
                load_symset("DECGraphics", PRIMARY);
                switch_symbols(TRUE);
            } else {
                raw_printf("Unknown option: %s", *argv);
            }
            break;
        case 'X':

            discover = TRUE, wizard = FALSE;
            break;
#ifdef NEWS
        case 'n':
            iflags.news = FALSE;
            break;
#endif
        case 'u':
            if (argv[0][2]) {
                (void) strncpy(plname, argv[0] + 2, sizeof plname - 1);
            } else if (argc > 1) {
                argc--;
                argv++;
                (void) strncpy(plname, argv[0], sizeof plname - 1);
            } else {
                raw_print("Player name expected after -u");
            }
            break;
        case 'I':
        case 'i':
            if (!strncmpi(*argv, "-IBMgraphics", l)) {
                load_symset("IBMGraphics", PRIMARY);
                load_symset("RogueIBM", ROGUESET);
                switch_symbols(TRUE);
            } else {
                raw_printf("Unknown option: %s", *argv);
            }
            break;
        case 'p': /* profession (role) */
            if (argv[0][2]) {
                if ((i = str2role(&argv[0][2])) >= 0)
                    flags.initrole = i;
            } else if (argc > 1) {
                argc--;
                argv++;
                if ((i = str2role(argv[0])) >= 0)
                    flags.initrole = i;
            }
            break;
        case 'r': /* race */
            if (argv[0][2]) {
                if ((i = str2race(&argv[0][2])) >= 0)
                    flags.initrace = i;
            } else if (argc > 1) {
                argc--;
                argv++;
                if ((i = str2race(argv[0])) >= 0)
                    flags.initrace = i;
            }
            break;
        case 'w': /* windowtype */
            config_error_init(FALSE, "command line", FALSE);
            choose_windows(&argv[0][2]);
            config_error_done();
            break;
        case '@':
            flags.randomall = 1;
            break;
        default:
            if ((i = str2role(&argv[0][1])) >= 0) {
                flags.initrole = i;
                break;
            }
            /* else raw_printf("Unknown option: %s", *argv); */
        }
    }

#ifdef SYSCF
    if (argc > 1)
        raw_printf("MAXPLAYERS are set in sysconf file.\n");
#else
    /* XXX This is deprecated in favor of SYSCF with MAXPLAYERS */
    if (argc > 1)
        locknum = atoi(argv[1]);
#endif
#ifdef MAX_NR_OF_PLAYERS
    /* limit to compile-time limit */
    if (!locknum || locknum > MAX_NR_OF_PLAYERS)
        locknum = MAX_NR_OF_PLAYERS;
#endif
#ifdef SYSCF
    /* let syscf override compile-time limit */
    if (!locknum || (sysopt.maxplayers && locknum > sysopt.maxplayers))
        locknum = sysopt.maxplayers;
#endif
}