int main(int argc, const char **argv)
{
    int force = 0;
    const char *stateFile = STATEFILE;
    const char *logFile = NULL;
    FILE *logFd = NULL;
    int rc = 0;
    int arg;
    const char **files;
    poptContext optCon;
    struct logInfo *log;

    struct poptOption options[] = {
        {"debug", 'd', 0, NULL, 'd',
            "Don't do anything, just test and print debug messages", NULL},
        {"force", 'f', 0, &force, 0, "Force file rotation", NULL},
        {"mail", 'm', POPT_ARG_STRING, &mailCommand, 0,
            "Command to send mail (instead of `" DEFAULT_MAIL_COMMAND "')",
            "command"},
        {"state", 's', POPT_ARG_STRING, &stateFile, 0,
            "Path of state file",
            "statefile"},
        {"verbose", 'v', 0, NULL, 'v', "Display messages during rotation", NULL},
        {"log", 'l', POPT_ARG_STRING, &logFile, 'l', "Log file or 'syslog' to log to syslog",
            "logfile"},
        {"version", '\0', POPT_ARG_NONE, NULL, 'V', "Display version information", NULL},
        POPT_AUTOHELP { NULL, 0, 0, NULL, 0, NULL, NULL }
    };

    logSetLevel(MESS_NORMAL);
    setlocale (LC_ALL, "");

    optCon = poptGetContext("logrotate", argc, argv, options, 0);
    poptReadDefaultConfig(optCon, 1);
    poptSetOtherOptionHelp(optCon, "[OPTION...] <configfile>");

    while ((arg = poptGetNextOpt(optCon)) >= 0) {
        switch (arg) {
            case 'd':
                debug = 1;
                message(MESS_NORMAL, "WARNING: logrotate in debug mode does nothing"
                        " except printing debug messages!  Consider using verbose"
                        " mode (-v) instead if this is not what you want.\n\n");
                /* fallthrough */
            case 'v':
                logSetLevel(MESS_DEBUG);
                break;
            case 'l':
                if (strcmp(logFile, "syslog") == 0) {
                    logToSyslog(1);
                }
                else {
                    logFd = fopen(logFile, "w");
                    if (!logFd) {
                        message(MESS_ERROR, "error opening log file %s: %s\n",
                                logFile, strerror(errno));
                        break;
                    }
                    logSetMessageFile(logFd);
                }
                break;
            case 'V':
                printf("logrotate %s\n", VERSION);
                printf("\n");
                printf("    Default mail command:       %s\n", DEFAULT_MAIL_COMMAND);
                printf("    Default compress command:   %s\n", COMPRESS_COMMAND);
                printf("    Default uncompress command: %s\n", UNCOMPRESS_COMMAND);
                printf("    Default compress extension: %s\n", COMPRESS_EXT);
                printf("    Default state file path:    %s\n", STATEFILE);
#ifdef WITH_ACL
                printf("    ACL support:                yes\n");
#else
                printf("    ACL support:                no\n");
#endif
#ifdef WITH_SELINUX
                printf("    SELinux support:            yes\n");
#else
                printf("    SELinux support:            no\n");
#endif
                poptFreeContext(optCon);
                exit(0);
            default:
                break;
        }
    }

    if (arg < -1) {
        fprintf(stderr, "logrotate: bad argument %s: %s\n",
                poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
                poptStrerror(rc));
        poptFreeContext(optCon);
        return 2;
    }

    files = poptGetArgs(optCon);
    if (!files) {
        fprintf(stderr, "logrotate " VERSION
                " - Copyright (C) 1995-2001 Red Hat, Inc.\n");
        fprintf(stderr,
                "This may be freely redistributed under the terms of "
                "the GNU General Public License\n\n");
        poptPrintUsage(optCon, stderr, 0);
        poptFreeContext(optCon);
        exit(1);
    }
#ifdef WITH_SELINUX
    selinux_enabled = (is_selinux_enabled() > 0);
    selinux_enforce = security_getenforce();
#endif

    TAILQ_INIT(&logs);

    if (readAllConfigPaths(files))
        rc = 1;

    poptFreeContext(optCon);
    nowSecs = time(NULL);

    if (readState(stateFile))
        rc = 1;

    message(MESS_DEBUG, "\nHandling %d logs\n", numLogs);

    for (log = logs.tqh_first; log != NULL; log = log->list.tqe_next)
        rc |= rotateLogSet(log, force);

    if (!debug)
        rc |= writeState(stateFile);

    return (rc != 0);
}