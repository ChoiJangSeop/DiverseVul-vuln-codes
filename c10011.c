int options_cmdline(char *arg1, char *arg2) {
    char *name;
    CONF_TYPE type;

#ifdef USE_WIN32
    (void)arg2; /* squash the unused parameter warning */
#endif
    if(!arg1) {
        name=
#ifdef CONFDIR
            CONFDIR
#ifdef USE_WIN32
            "\\"
#else
            "/"
#endif
#endif
            "stunnel.conf";
        type=CONF_FILE;
    } else if(!strcasecmp(arg1, "-help")) {
        parse_global_option(CMD_PRINT_HELP, NULL, NULL);
        parse_service_option(CMD_PRINT_HELP, NULL, NULL, NULL);
        log_flush(LOG_MODE_INFO);
        return 2;
    } else if(!strcasecmp(arg1, "-version")) {
        parse_global_option(CMD_PRINT_DEFAULTS, NULL, NULL);
        parse_service_option(CMD_PRINT_DEFAULTS, NULL, NULL, NULL);
        log_flush(LOG_MODE_INFO);
        return 2;
    } else if(!strcasecmp(arg1, "-sockets")) {
        socket_options_print();
        log_flush(LOG_MODE_INFO);
        return 2;
    } else if(!strcasecmp(arg1, "-options")) {
        print_ssl_options();
        log_flush(LOG_MODE_INFO);
        return 2;
    } else
#ifndef USE_WIN32
    if(!strcasecmp(arg1, "-fd")) {
        if(!arg2) {
            s_log(LOG_ERR, "No file descriptor specified");
            print_syntax();
            return 1;
        }
        name=arg2;
        type=CONF_FD;
    } else
#endif
    {
        name=arg1;
        type=CONF_FILE;
    }

    if(type==CONF_FILE) {
#ifdef HAVE_REALPATH
        char *real_path=NULL;
#ifdef MAXPATHLEN
        /* a workaround for pre-POSIX.1-2008 4.4BSD and Solaris */
        real_path=malloc(MAXPATHLEN);
#endif
        real_path=realpath(name, real_path);
        if(!real_path) {
            s_log(LOG_ERR, "Invalid configuration file name \"%s\"", name);
            ioerror("realpath");
            return 1;
        }
        configuration_file=str_dup(real_path);
        free(real_path);
#else
        configuration_file=str_dup(name);
#endif
#ifndef USE_WIN32
    } else if(type==CONF_FD) {
        configuration_file=str_dup(name);
#endif
    }
    return options_parse(type);
}