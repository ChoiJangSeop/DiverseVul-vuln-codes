get_one_option(int optid, const struct my_option *opt,
               char *argument)
{
  my_bool add_option= TRUE;

  switch (optid) {

  case '?':
    printf("%s  Ver %s Distrib %s, for %s (%s)\n",
           my_progname, VER, MYSQL_SERVER_VERSION, SYSTEM_TYPE, MACHINE_TYPE);
    puts(ORACLE_WELCOME_COPYRIGHT_NOTICE("2000"));
    puts("MySQL utility for upgrading databases to new MySQL versions.\n");
    my_print_help(my_long_options);
    exit(0);
    break;

  case '#':
    DBUG_PUSH(argument ? argument : default_dbug_option);
    add_option= FALSE;
    debug_check_flag= 1;
    break;

  case 'p':
    if (argument == disabled_my_option)
      argument= (char*) "";			/* Don't require password */
    tty_password= 1;
    add_option= FALSE;
    if (argument)
    {
      /* Add password to ds_args before overwriting the arg with x's */
      add_one_option(&ds_args, opt, argument);
      while (*argument)
        *argument++= 'x';                       /* Destroy argument */
      tty_password= 0;
    }
    break;

  case 't':
    strnmov(opt_tmpdir, argument, sizeof(opt_tmpdir));
    add_option= FALSE;
    break;

  case 'b': /* --basedir   */
  case 'd': /* --datadir   */
    fprintf(stderr, "%s: the '--%s' option is always ignored\n",
            my_progname, optid == 'b' ? "basedir" : "datadir");
    /* FALLTHROUGH */

  case 'k':                                     /* --version-check */
  case 'v': /* --verbose   */
  case 'f': /* --force     */
  case 's':                                     /* --upgrade-system-tables */
  case OPT_WRITE_BINLOG:                        /* --write-binlog */
    add_option= FALSE;
    break;

  case 'h': /* --host */
  case 'W': /* --pipe */
  case 'P': /* --port */
  case 'S': /* --socket */
  case OPT_MYSQL_PROTOCOL: /* --protocol */
  case OPT_SHARED_MEMORY_BASE_NAME: /* --shared-memory-base-name */
  case OPT_PLUGIN_DIR:                          /* --plugin-dir */
  case OPT_DEFAULT_AUTH:                        /* --default-auth */
    add_one_option(&conn_args, opt, argument);
    break;
  }

  if (add_option)
  {
    /*
      This is an option that is accpted by mysql_upgrade just so
      it can be passed on to "mysql" and "mysqlcheck"
      Save it in the ds_args string
    */
    add_one_option(&ds_args, opt, argument);
  }
  return 0;
}