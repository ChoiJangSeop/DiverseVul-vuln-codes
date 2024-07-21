int main(int argc, char **argv)
{
    int opt, i, r;
    int dousers = 0;
    int rflag = 0;
    int mflag = 0;
    int fflag = 0;
    int xflag = 0;
    struct buf buf = BUF_INITIALIZER;
    char *alt_config = NULL;
    char *start_part = NULL;
    struct reconstruct_rock rrock = { NULL, HASH_TABLE_INITIALIZER };

    progname = basename(argv[0]);

    while ((opt = getopt(argc, argv, "C:kp:rmfsxdgGqRUMIoOnV:u")) != EOF) {
        switch (opt) {
        case 'C': /* alt config file */
            alt_config = optarg;
            break;

        case 'p':
            start_part = optarg;
            break;

        case 'r':
            rflag = 1;
            break;

        case 'u':
            dousers = 1;
            break;

        case 'm':
            mflag = 1;
            break;

        case 'n':
            reconstruct_flags &= ~RECONSTRUCT_MAKE_CHANGES;
            break;

        case 'g':
            fprintf(stderr, "reconstruct: deprecated option -g ignored\n");
            break;

        case 'G':
            reconstruct_flags |= RECONSTRUCT_ALWAYS_PARSE;
            break;

        case 'f':
            fflag = 1;
            break;

        case 'x':
            xflag = 1;
            break;

        case 'k':
            fprintf(stderr, "reconstruct: deprecated option -k ignored\n");
            break;

        case 's':
            reconstruct_flags &= ~RECONSTRUCT_DO_STAT;
            break;

        case 'q':
            reconstruct_flags |= RECONSTRUCT_QUIET;
            break;

        case 'R':
            reconstruct_flags |= RECONSTRUCT_GUID_REWRITE;
            break;

        case 'U':
            reconstruct_flags |= RECONSTRUCT_GUID_UNLINK;
            break;

        case 'o':
            reconstruct_flags |= RECONSTRUCT_IGNORE_ODDFILES;
            break;

        case 'O':
            reconstruct_flags |= RECONSTRUCT_REMOVE_ODDFILES;
            break;

        case 'M':
            reconstruct_flags |= RECONSTRUCT_PREFER_MBOXLIST;
            break;

        case 'I':
            updateuniqueids = 1;
            break;

        case 'V':
            if (!strcasecmp(optarg, "max"))
                setversion = MAILBOX_MINOR_VERSION;
            else
                setversion = atoi(optarg);
            break;

        default:
            usage();
        }
    }

    cyrus_init(alt_config, "reconstruct", 0, CONFIG_NEED_PARTITION_DATA);
    global_sasl_init(1,0,NULL);

    /* Set namespace -- force standard (internal) */
    if ((r = mboxname_init_namespace(&recon_namespace, 1)) != 0) {
        syslog(LOG_ERR, "%s", error_message(r));
        fatal(error_message(r), EX_CONFIG);
    }

    signals_set_shutdown(&shut_down);
    signals_add_handlers(0);

    if (mflag) {
        if (rflag || fflag || optind != argc) {
            cyrus_done();
            usage();
        }
        do_mboxlist();
    }

    mbentry_t mbentry = MBENTRY_INITIALIZER;
    unsigned flags = !xflag ? MBOXLIST_CREATE_DBONLY : 0;

    /* Deal with nonexistent mailboxes */
    if (start_part) {
        /* We were handed a mailbox that does not exist currently */
        if(optind == argc) {
            fprintf(stderr,
                    "When using -p, you must specify a mailbox to attempt to reconstruct.");
            exit(EX_USAGE);
        }

        /* do any of the mailboxes exist in mboxlist already? */
        /* Do they look like mailboxes? */
        for (i = optind; i < argc; i++) {
            if (strchr(argv[i],'%') || strchr(argv[i],'*')) {
                fprintf(stderr, "Using wildcards with -p is not supported.\n");
                exit(EX_USAGE);
            }

            /* Translate mailboxname */
            char *intname = mboxname_from_external(argv[i], &recon_namespace, NULL);

            /* Does it exist */
            do {
                r = mboxlist_lookup(intname, NULL, NULL);
            } while (r == IMAP_AGAIN);

            if (r != IMAP_MAILBOX_NONEXISTENT) {
                fprintf(stderr,
                        "Mailbox %s already exists.  Cannot specify -p.\n",
                        argv[i]);
                exit(EX_USAGE);
            }
            free(intname);
        }

        /* None of them exist.  Create them. */
        for (i = optind; i < argc; i++) {
            /* Translate mailboxname */
            char *intname = mboxname_from_external(argv[i], &recon_namespace, NULL);

            /* don't notify mailbox creation here */
            mbentry.name = intname;
            mbentry.mbtype = MBTYPE_EMAIL;
            mbentry.partition = start_part;
            r = mboxlist_createmailboxlock(&mbentry, 0/*options*/, 0/*highestmodseq*/,
                                       1/*isadmin*/, NULL/*userid*/, NULL/*authstate*/,
                                       flags, NULL/*mailboxptr*/);
            if (r) {
                fprintf(stderr, "could not create %s\n", argv[i]);
            }

            free(intname);
        }
    }

    /* set up reconstruct rock */
    if (fflag) rrock.discovered = strarray_new();
    construct_hash_table(&rrock.visited, 2047, 1); /* XXX magic numbers */

    /* Normal Operation */
    if (optind == argc) {
        if (rflag || dousers) {
            fprintf(stderr, "please specify a mailbox to recurse from\n");
            cyrus_done();
            exit(EX_USAGE);
        }
        assert(!rflag);
        buf_setcstr(&buf, "*");
        mboxlist_findall(&recon_namespace, buf_cstring(&buf), 1, 0, 0,
                         do_reconstruct, &rrock);
    }

    for (i = optind; i < argc; i++) {
        if (dousers) {
            mboxlist_usermboxtree(argv[i], NULL, do_reconstruct_p, &rrock,
                                  MBOXTREE_TOMBSTONES|MBOXTREE_DELETED);
            continue;
        }
        char *domain = NULL;

        /* save domain */
        if (config_virtdomains) domain = strchr(argv[i], '@');

        buf_setcstr(&buf, argv[i]);

        /* reconstruct the first mailbox/pattern */
        mboxlist_findall(&recon_namespace, buf_cstring(&buf), 1, 0, 0, do_reconstruct, &rrock);

        if (rflag) {
            /* build a pattern for submailboxes */
            int atidx = buf_findchar(&buf, 0, '@');
            if (atidx >= 0)
                buf_truncate(&buf, atidx);
            buf_putc(&buf, recon_namespace.hier_sep);
            buf_putc(&buf, '*');

            /* append the domain */
            if (domain) buf_appendcstr(&buf, domain);

            /* reconstruct the submailboxes */
            mboxlist_findall(&recon_namespace, buf_cstring(&buf), 1, 0, 0, do_reconstruct, &rrock);
        }
    }

    /* examine our list to see if we discovered anything */
    while (rrock.discovered && rrock.discovered->count) {
        char *name = strarray_shift(rrock.discovered);
        int r = 0;

        /* create p (database only) and reconstruct it */
        /* partition is defined by the parent mailbox */
        /* don't notify mailbox creation here */
        mbentry.name = name;
        mbentry.mbtype = MBTYPE_EMAIL;
        mbentry.partition = NULL;
        r = mboxlist_createmailboxlock(&mbentry, 0/*options*/, 0/*highestmodseq*/,
                                       1/*isadmin*/, NULL/*userid*/, NULL/*authstate*/,
                                       flags, NULL/*mailboxptr*/);
        if (r) {
            fprintf(stderr, "createmailbox %s: %s\n",
                    name, error_message(r));
        } else {
            mboxlist_findone(&recon_namespace, name, 1, 0, 0, do_reconstruct, &rrock);
        }
        /* may have added more things into our list */

        free(name);
    }

    if (rrock.discovered) strarray_free(rrock.discovered);
    free_hash_table(&rrock.visited, NULL);

    buf_free(&buf);

    partlist_local_done();

    libcyrus_run_delayed();

    shut_down(0);
}