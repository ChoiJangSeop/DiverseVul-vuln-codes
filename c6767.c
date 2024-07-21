main(
    int		argc,
    char **	argv)
{
    char *line = NULL;
    char *qdisk = NULL;
    char *qamdevice = NULL;
    char *optstr = NULL;
    char *err_extra = NULL;
    char *s, *fp;
    int ch;
    dle_t *dle;
    int level;
    GSList *errlist;
    am_level_t *alevel;

    if (argc > 1 && argv && argv[1] && g_str_equal(argv[1], "--version")) {
	printf("selfcheck-%s\n", VERSION);
	return (0);
    }

    /* initialize */

    /*
     * Configure program for internationalization:
     *   1) Only set the message locale for now.
     *   2) Set textdomain for all amanda related programs to "amanda"
     *      We don't want to be forced to support dozens of message catalogs.
     */  
    setlocale(LC_MESSAGES, "C");
    textdomain("amanda"); 

    safe_fd(-1, 0);
    openbsd_fd_inform();
    safe_cd();

    set_pname("selfcheck");

    /* Don't die when child closes pipe */
    signal(SIGPIPE, SIG_IGN);

    add_amanda_log_handler(amanda_log_stderr);
    add_amanda_log_handler(amanda_log_syslog);
    dbopen(DBG_SUBDIR_CLIENT);
    startclock();
    dbprintf(_("version %s\n"), VERSION);
    g_printf("OK version %s\n", VERSION);
    print_platform();

    if(argc > 2 && strcmp(argv[1], "amandad") == 0) {
	amandad_auth = stralloc(argv[2]);
    }

    config_init(CONFIG_INIT_CLIENT, NULL);
    /* (check for config errors comes later) */

    check_running_as(RUNNING_AS_CLIENT_LOGIN);

    our_features = am_init_feature_set();
    our_feature_string = am_feature_to_string(our_features);

    /* handle all service requests */

    /*@ignore@*/
    for(; (line = agets(stdin)) != NULL; free(line)) {
    /*@end@*/
	if (line[0] == '\0')
	    continue;

	if(strncmp_const(line, "OPTIONS ") == 0) {
	    g_options = parse_g_options(line+8, 1);
	    if(!g_options->hostname) {
		g_options->hostname = alloc(MAX_HOSTNAME_LENGTH+1);
		gethostname(g_options->hostname, MAX_HOSTNAME_LENGTH);
		g_options->hostname[MAX_HOSTNAME_LENGTH] = '\0';
	    }

	    g_printf("OPTIONS ");
	    if(am_has_feature(g_options->features, fe_rep_options_features)) {
		g_printf("features=%s;", our_feature_string);
	    }
	    if(am_has_feature(g_options->features, fe_rep_options_hostname)) {
		g_printf("hostname=%s;", g_options->hostname);
	    }
	    g_printf("\n");
	    fflush(stdout);

	    if (g_options->config) {
		/* overlay this configuration on the existing (nameless) configuration */
		config_init(CONFIG_INIT_CLIENT | CONFIG_INIT_EXPLICIT_NAME | CONFIG_INIT_OVERLAY,
			    g_options->config);

		dbrename(get_config_name(), DBG_SUBDIR_CLIENT);
	    }

	    /* check for any config errors now */
	    if (config_errors(&errlist) >= CFGERR_ERRORS) {
		char *errstr = config_errors_to_error_string(errlist);
		g_printf("%s\n", errstr);
		dbclose();
		return 1;
	    }

	    if (am_has_feature(g_options->features, fe_req_xml)) {
		break;
	    }
	    continue;
	}

	dle = alloc_dle();
	s = line;
	ch = *s++;

	skip_whitespace(s, ch);			/* find program name */
	if (ch == '\0') {
	    goto err;				/* no program */
	}
	dle->program = s - 1;
	skip_non_whitespace(s, ch);
	s[-1] = '\0';				/* terminate the program name */

	dle->program_is_application_api = 0;
	if(strcmp(dle->program,"APPLICATION")==0) {
	    dle->program_is_application_api = 1;
	    skip_whitespace(s, ch);		/* find dumper name */
	    if (ch == '\0') {
		goto err;			/* no program */
	    }
	    dle->program = s - 1;
	    skip_non_whitespace(s, ch);
	    s[-1] = '\0';			/* terminate the program name */
	}

	if(strncmp_const(dle->program, "CALCSIZE") == 0) {
	    skip_whitespace(s, ch);		/* find program name */
	    if (ch == '\0') {
		goto err;			/* no program */
	    }
	    dle->program = s - 1;
	    skip_non_whitespace(s, ch);
	    s[-1] = '\0';
	    dle->estimatelist = g_slist_append(dle->estimatelist,
					       GINT_TO_POINTER(ES_CALCSIZE));
	}
	else {
	    dle->estimatelist = g_slist_append(dle->estimatelist,
					       GINT_TO_POINTER(ES_CLIENT));
	}

	skip_whitespace(s, ch);			/* find disk name */
	if (ch == '\0') {
	    goto err;				/* no disk */
	}
	qdisk = s - 1;
	skip_quoted_string(s, ch);
	s[-1] = '\0';				/* terminate the disk name */
	dle->disk = unquote_string(qdisk);

	skip_whitespace(s, ch);                 /* find the device or level */
	if (ch == '\0') {
	    goto err;				/* no device or level */
	}
	if(!isdigit((int)s[-1])) {
	    fp = s - 1;
	    skip_quoted_string(s, ch);
	     s[-1] = '\0';			/* terminate the device */
	    qamdevice = stralloc(fp);
	    dle->device = unquote_string(qamdevice);
	    skip_whitespace(s, ch);		/* find level number */
	}
	else {
	    dle->device = stralloc(dle->disk);
	    qamdevice = stralloc(qdisk);
	}

						/* find level number */
	if (ch == '\0' || sscanf(s - 1, "%d", &level) != 1) {
	    goto err;				/* bad level */
	}
	alevel = g_new0(am_level_t, 1);
	alevel->level = level;
	dle->levellist = g_slist_append(dle->levellist, alevel);
	skip_integer(s, ch);

	skip_whitespace(s, ch);
	if (ch && strncmp_const_skip(s - 1, "OPTIONS ", s, ch) == 0) {
	    skip_whitespace(s, ch);		/* find the option string */
	    if(ch == '\0') {
		goto err;			/* bad options string */
	    }
	    optstr = s - 1;
	    skip_quoted_string(s, ch);
	    s[-1] = '\0';			/* terminate the options */
	    parse_options(optstr, dle, g_options->features, 1);
	    /*@ignore@*/

	    check_options(dle);
	    check_disk(dle);

	    /*@end@*/
	} else if (ch == '\0') {
	    /* check all since no option */
	    need_samba=1;
	    need_rundump=1;
	    need_dump=1;
	    need_restore=1;
	    need_vdump=1;
	    need_vrestore=1;
	    need_xfsdump=1;
	    need_xfsrestore=1;
	    need_vxdump=1;
	    need_vxrestore=1;
	    need_runtar=1;
	    need_gnutar=1;
	    need_compress_path=1;
	    need_calcsize=1;
	    need_global_check=1;
	    /*@ignore@*/
	    check_disk(dle);
	    /*@end@*/
	} else {
	    goto err;				/* bad syntax */
	}
	amfree(qamdevice);
    }
    if (g_options == NULL) {
	g_printf(_("ERROR [Missing OPTIONS line in selfcheck input]\n"));
	error(_("Missing OPTIONS line in selfcheck input\n"));
	/*NOTREACHED*/
    }

    if (am_has_feature(g_options->features, fe_req_xml)) {
	char  *errmsg = NULL;
	dle_t *dles, *dle, *dle_next;

	dles = amxml_parse_node_FILE(stdin, &errmsg);
	if (errmsg) {
	    err_extra = errmsg;
	    goto err;
	}
	if (merge_dles_properties(dles, 1) == 0) {
	    goto checkoverall;
	}
	for (dle = dles; dle != NULL; dle = dle->next) {
	    run_client_scripts(EXECUTE_ON_PRE_HOST_AMCHECK, g_options, dle,
			       stdout);
	}
	for (dle = dles; dle != NULL; dle = dle->next) {
	    check_options(dle);
	    run_client_scripts(EXECUTE_ON_PRE_DLE_AMCHECK, g_options, dle,
			       stdout);
	    check_disk(dle);
	    run_client_scripts(EXECUTE_ON_POST_DLE_AMCHECK, g_options, dle,
			       stdout);
	}
	for (dle = dles; dle != NULL; dle = dle->next) {
	    run_client_scripts(EXECUTE_ON_POST_HOST_AMCHECK, g_options, dle,
			       stdout);
	}
	for (dle = dles; dle != NULL; dle = dle_next) {
	    dle_next = dle->next;
	    free_dle(dle);
	}
    }

checkoverall:
    check_overall();

    amfree(line);
    amfree(our_feature_string);
    am_release_feature_set(our_features);
    our_features = NULL;
    free_g_options(g_options);

    dbclose();
    return 0;

 err:
    if (err_extra) {
	g_printf(_("ERROR [FORMAT ERROR IN REQUEST PACKET %s]\n"), err_extra);
	dbprintf(_("REQ packet is bogus: %s\n"), err_extra);
    } else {
	g_printf(_("ERROR [FORMAT ERROR IN REQUEST PACKET]\n"));
	dbprintf(_("REQ packet is bogus\n"));
    }
    dbclose();
    return 1;
}