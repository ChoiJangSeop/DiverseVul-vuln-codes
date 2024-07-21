netsnmp_init_mib(void)
{
    const char     *prefix;
    char           *env_var, *entry;
    PrefixListPtr   pp = &mib_prefixes[0];
    char           *st = NULL;

    if (Mib)
        return;
    netsnmp_init_mib_internals();

    /*
     * Initialise the MIB directory/ies 
     */
    netsnmp_fixup_mib_directory();
    env_var = strdup(netsnmp_get_mib_directory());
    if (!env_var)
        return;
    netsnmp_mibindex_load();

    DEBUGMSGTL(("init_mib",
                "Seen MIBDIRS: Looking in '%s' for mib dirs ...\n",
                env_var));

    entry = strtok_r(env_var, ENV_SEPARATOR, &st);
    while (entry) {
        add_mibdir(entry);
        entry = strtok_r(NULL, ENV_SEPARATOR, &st);
    }
    SNMP_FREE(env_var);

    env_var = netsnmp_getenv("MIBFILES");
    if (env_var != NULL) {
        if (*env_var == '+')
            entry = strtok_r(env_var+1, ENV_SEPARATOR, &st);
        else
            entry = strtok_r(env_var, ENV_SEPARATOR, &st);
        while (entry) {
            add_mibfile(entry, NULL, NULL);
            entry = strtok_r(NULL, ENV_SEPARATOR, &st);
        }
    }

    netsnmp_init_mib_internals();

    /*
     * Read in any modules or mibs requested 
     */

    env_var = netsnmp_getenv("MIBS");
    if (env_var == NULL) {
        if (confmibs != NULL)
            env_var = strdup(confmibs);
        else
            env_var = strdup(NETSNMP_DEFAULT_MIBS);
    } else {
        env_var = strdup(env_var);
    }
    if (env_var && ((*env_var == '+') || (*env_var == '-'))) {
        entry =
            (char *) malloc(strlen(NETSNMP_DEFAULT_MIBS) + strlen(env_var) + 2);
        if (!entry) {
            DEBUGMSGTL(("init_mib", "env mibs malloc failed"));
            SNMP_FREE(env_var);
            return;
        } else {
            if (*env_var == '+')
                sprintf(entry, "%s%c%s", NETSNMP_DEFAULT_MIBS, ENV_SEPARATOR_CHAR,
                        env_var+1);
            else
                sprintf(entry, "%s%c%s", env_var+1, ENV_SEPARATOR_CHAR,
                        NETSNMP_DEFAULT_MIBS );
        }
        SNMP_FREE(env_var);
        env_var = entry;
    }

    DEBUGMSGTL(("init_mib",
                "Seen MIBS: Looking in '%s' for mib files ...\n",
                env_var));
    entry = strtok_r(env_var, ENV_SEPARATOR, &st);
    while (entry) {
        if (strcasecmp(entry, DEBUG_ALWAYS_TOKEN) == 0) {
            read_all_mibs();
        } else if (strstr(entry, "/") != NULL) {
            read_mib(entry);
        } else {
            netsnmp_read_module(entry);
        }
        entry = strtok_r(NULL, ENV_SEPARATOR, &st);
    }
    adopt_orphans();
    SNMP_FREE(env_var);

    env_var = netsnmp_getenv("MIBFILES");
    if (env_var != NULL) {
        if ((*env_var == '+') || (*env_var == '-')) {
#ifdef NETSNMP_DEFAULT_MIBFILES
            entry =
                (char *) malloc(strlen(NETSNMP_DEFAULT_MIBFILES) +
                                strlen(env_var) + 2);
            if (!entry) {
                DEBUGMSGTL(("init_mib", "env mibfiles malloc failed"));
            } else {
                if (*env_var++ == '+')
                    sprintf(entry, "%s%c%s", NETSNMP_DEFAULT_MIBFILES, ENV_SEPARATOR_CHAR,
                            env_var );
                else
                    sprintf(entry, "%s%c%s", env_var, ENV_SEPARATOR_CHAR,
                            NETSNMP_DEFAULT_MIBFILES );
            }
            SNMP_FREE(env_var);
            env_var = entry;
#else
            env_var = strdup(env_var + 1);
#endif
        } else {
            env_var = strdup(env_var);
        }
    } else {
#ifdef NETSNMP_DEFAULT_MIBFILES
        env_var = strdup(NETSNMP_DEFAULT_MIBFILES);
#endif
    }

    if (env_var != NULL) {
        DEBUGMSGTL(("init_mib",
                    "Seen MIBFILES: Looking in '%s' for mib files ...\n",
                    env_var));
        entry = strtok_r(env_var, ENV_SEPARATOR, &st);
        while (entry) {
            read_mib(entry);
            entry = strtok_r(NULL, ENV_SEPARATOR, &st);
        }
        SNMP_FREE(env_var);
    }

    prefix = netsnmp_getenv("PREFIX");

    if (!prefix)
        prefix = Standard_Prefix;

    Prefix = (char *) malloc(strlen(prefix) + 2);
    if (!Prefix)
        DEBUGMSGTL(("init_mib", "Prefix malloc failed"));
    else
        strcpy(Prefix, prefix);

    DEBUGMSGTL(("init_mib",
                "Seen PREFIX: Looking in '%s' for prefix ...\n", Prefix));

    /*
     * remove trailing dot 
     */
    if (Prefix) {
        env_var = &Prefix[strlen(Prefix) - 1];
        if (*env_var == '.')
            *env_var = '\0';
    }

    pp->str = Prefix;           /* fixup first mib_prefix entry */
    /*
     * now that the list of prefixes is built, save each string length. 
     */
    while (pp->str) {
        pp->len = strlen(pp->str);
        pp++;
    }

    Mib = tree_head;            /* Backwards compatibility */
    tree_top = (struct tree *) calloc(1, sizeof(struct tree));
    /*
     * XX error check ? 
     */
    if (tree_top) {
        tree_top->label = strdup("(top)");
        tree_top->child_list = tree_head;
    }
}