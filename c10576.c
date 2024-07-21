main(int argc, char** argv)
{
	char *		cmdname;
	int		rc;
	Stonith *	s;
	const char *	SwitchType = NULL;
	const char *	tmp;
	const char *	optfile = NULL;
	const char *	parameters = NULL;
	int		reset_type = ST_GENERIC_RESET;
	int		verbose = 0;
	int		status = 0;
	int		silent = 0;
	int		listhosts = 0;
	int		listtypes = 0;
	int 		listparanames = 0;

	int		c;
	int		errors = 0;
	int		argcount;
	StonithNVpair	nvargs[MAXNVARG];
	int		nvcount=0;
	int		j;
	int		count = 1;
	int		help = 0;
	int		metadata = 0;

	/* The bladehpi stonith plugin makes use of openhpi which is
	 * threaded.  The mix of memory allocation without thread
	 * initialization followed by g_thread_init followed by
	 * deallocating that memory results in segfault.  Hence the
	 * following G_SLICE setting; see
	 * http://library.gnome.org/devel/glib/stable/glib-Memory-Slices.html#g-slice-alloc
	 */

	setenv("G_SLICE", "always-malloc", 1);

	if ((cmdname = strrchr(argv[0], '/')) == NULL) {
		cmdname = argv[0];
	}else{
		++cmdname;
	}


	while ((c = getopt(argc, argv, OPTIONS)) != -1) {
		switch(c) {

		case 'c':	count = atoi(optarg);
				if (count < 1) {
					fprintf(stderr
					,	"bad count [%s]\n"
					,	optarg);
					usage(cmdname, 1, NULL);
				}
				break;

		case 'd':	debug++;
				break;

		case 'F':	optfile = optarg;
				break;

		case 'h':	help++;
				break;

		case 'm':	metadata++;
				break;

		case 'l':	++listhosts;
				break;

		case 'L':	++listtypes;
				break;

		case 'p':	parameters = optarg;
				break;

		case 's':	++silent;
				break;

		case 'S':	++status;
				break;

		case 't':	SwitchType = optarg;
				break;

		case 'T':	if (strcmp(optarg, "on")== 0) {
					reset_type = ST_POWERON;
				}else if (strcmp(optarg, "off")== 0) {
					reset_type = ST_POWEROFF;
				}else if (strcmp(optarg, "reset")== 0) {
					reset_type = ST_GENERIC_RESET;
				}else{
					fprintf(stderr
					,	"bad reset type [%s]\n"
					,	optarg);
					usage(cmdname, 1, NULL);
				}
				break;

		case 'n':	++listparanames;
				break;

		case 'v':	++verbose;
				break;

		default:	++errors;
				break;
		}
	}

	if (help && !errors) {
		usage(cmdname, 0, SwitchType);
	}
	if (debug) {
		PILpisysSetDebugLevel(debug);
		setenv("HA_debug","2",0);
	}
	if (optfile && parameters) {
		fprintf(stderr
		,	"Cannot include both -F and -p options\n");
		usage(cmdname, 1, NULL);
	}

	/*
	 *	Process name=value arguments on command line...
	 */
	for (;optind < argc; ++optind) {
		char *	eqpos;
		if ((eqpos=strchr(argv[optind], EQUAL)) == NULL) {
			break;
		}
		if (parameters)  {
			fprintf(stderr
			,	"Cannot include both -p and name=value "
			"style arguments\n");
			usage(cmdname, 1, NULL);
		}
		if (optfile)  {
			fprintf(stderr
			,	"Cannot include both -F and name=value "
			"style arguments\n");
			usage(cmdname, 1, NULL);
		}
		if (nvcount >= MAXNVARG) {
			fprintf(stderr
			,	"Too many name=value style arguments\n");
			exit(1);
		}
		nvargs[nvcount].s_name = argv[optind];
		*eqpos = EOS;
		nvargs[nvcount].s_value = eqpos+1;
		nvcount++;
	}
	nvargs[nvcount].s_name = NULL;
	nvargs[nvcount].s_value = NULL;

	argcount = argc - optind;

	if (!(argcount == 1 || (argcount < 1
	&&	(status||listhosts||listtypes||listparanames||metadata)))) {
		++errors;
	}

	if (errors) {
		usage(cmdname, 1, NULL);
	}

	if (listtypes) {
		char **	typelist;

		typelist = stonith_types();
		if (typelist == NULL) {
			syslog(LOG_ERR, "Could not list Stonith types.");
		}else{
			char **	this;

			for(this=typelist; *this; ++this) {
				printf("%s\n", *this);
			}
		}
		exit(0);
	}

#ifndef LOG_PERROR
#	define LOG_PERROR	0
#endif
	openlog(cmdname, (LOG_CONS|(silent ? 0 : LOG_PERROR)), LOG_USER);
	if (SwitchType == NULL) {
		fprintf(stderr,	"Must specify device type (-t option)\n");
		usage(cmdname, 1, NULL);
	}
	s = stonith_new(SwitchType);
	if (s == NULL) {
		syslog(LOG_ERR, "Invalid device type: '%s'", SwitchType);
		exit(S_OOPS);
	}
	if (debug) {
		stonith_set_debug(s, debug);
	}

	if (!listparanames && !metadata && optfile == NULL && parameters == NULL && nvcount == 0) {
		const char**	names;
		int		needs_parms = 1;

		if (s != NULL && (names = stonith_get_confignames(s)) != NULL && names[0] == NULL) {
			needs_parms = 0;
		}

		if (needs_parms) {
			fprintf(stderr
			,	"Must specify either -p option, -F option or "
			"name=value style arguments\n");
			if (s != NULL) {
				stonith_delete(s); 
			}
			usage(cmdname, 1, NULL);
		}
	}

	if (listparanames) {
		const char**	names;
		int		i;
		names = stonith_get_confignames(s);

		if (names != NULL) {
			for (i=0; names[i]; ++i) {
				printf("%s  ", names[i]);
			}
		}
		printf("\n");
		stonith_delete(s); 
		s=NULL;
		exit(0);
	}

	if (metadata) {
		print_stonith_meta(s,SwitchType);
		stonith_delete(s); 
		s=NULL;
		exit(0);
	}

	/* Old STONITH version 1 stuff... */
	if (optfile) {
		/* Configure the Stonith object from a file */
		if ((rc=stonith_set_config_file(s, optfile)) != S_OK) {
			syslog(LOG_ERR
			,	"Invalid config file for %s device."
			,	SwitchType);
#if 0
			syslog(LOG_INFO, "Config file syntax: %s"
			,	s->s_ops->getinfo(s, ST_CONF_FILE_SYNTAX));
#endif
			stonith_delete(s); s=NULL;
			exit(S_BADCONFIG);
		}
	}else if (parameters) {
		/* Configure Stonith object from the -p argument */
		StonithNVpair *		pairs;
		if ((pairs = stonith1_compat_string_to_NVpair
		     (	s, parameters)) == NULL) {
			fprintf(stderr
			,	"Invalid STONITH -p parameter [%s]\n"
			,	parameters);
			stonith_delete(s); s=NULL;
			exit(1);
		}
		if ((rc = stonith_set_config(s, pairs)) != S_OK) {
			fprintf(stderr
			,	"Invalid config info for %s device"
			,	SwitchType);
		}
	}else{
		/*
		 *	Configure STONITH device using cmdline arguments...
		 */
		if ((rc = stonith_set_config(s, nvargs)) != S_OK) {
			const char**	names;
			int		j;
			fprintf(stderr
			,	"Invalid config info for %s device\n"
			,	SwitchType);

			names = stonith_get_confignames(s);

			if (names != NULL) {
				fprintf(stderr
				,	"Valid config names are:\n");
			
				for (j=0; names[j]; ++j) {
					fprintf(stderr
					,	"\t%s\n", names[j]);
				}
			}
			stonith_delete(s); s=NULL;
			exit(rc);
		}
	}


	for (j=0; j < count; ++j) {
		rc = stonith_get_status(s);

		if ((tmp = stonith_get_info(s, ST_DEVICEID)) == NULL) {
			SwitchType = tmp;
		}

		if (status && !silent) {
			if (rc == S_OK) {
				syslog(LOG_ERR, "%s device OK.", SwitchType);
			}else{
				/* Uh-Oh */
				syslog(LOG_ERR, "%s device not accessible."
				,	SwitchType);
			}
		}

		if (listhosts) {
			char **	hostlist;

			hostlist = stonith_get_hostlist(s);
			if (hostlist == NULL) {
				syslog(LOG_ERR, "Could not list hosts for %s."
				,	SwitchType);
			}else{
				char **	this;

				for(this=hostlist; *this; ++this) {
					printf("%s\n", *this);
				}
				stonith_free_hostlist(hostlist);
			}
		}

		if (optind < argc) {
			char *nodename;
			nodename = g_strdup(argv[optind]);
			g_strdown(nodename);
			rc = stonith_req_reset(s, reset_type, nodename);
			g_free(nodename);
		}
	}
	stonith_delete(s); s = NULL;
	return(rc);
}