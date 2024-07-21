int main(int argc,char *argv[])
{
  int error= 0, ho_error;
  int first_command;
  my_bool can_handle_passwords;
  MYSQL mysql;
  char **commands, **save_argv;

  MY_INIT(argv[0]);
  mysql_init(&mysql);
  my_getopt_use_args_separator= TRUE;
  if (load_defaults("my",load_default_groups,&argc,&argv))
   exit(1); 
  my_getopt_use_args_separator= FALSE;

  save_argv = argv;				/* Save for free_defaults */
  if ((ho_error=handle_options(&argc, &argv, my_long_options, get_one_option)))
  {
    free_defaults(save_argv);
    exit(ho_error);
  }
  if (debug_info_flag)
    my_end_arg= MY_CHECK_ERROR | MY_GIVE_INFO;
  if (debug_check_flag)
    my_end_arg= MY_CHECK_ERROR;

  if (argc == 0)
  {
    usage();
    exit(1);
  }
  commands = argv;
  if (tty_password)
    opt_password = get_tty_password(NullS);

  (void) signal(SIGINT,endprog);			/* Here if abort */
  (void) signal(SIGTERM,endprog);		/* Here if abort */

  if (opt_bind_addr)
    mysql_options(&mysql,MYSQL_OPT_BIND,opt_bind_addr);
  if (opt_compress)
    mysql_options(&mysql,MYSQL_OPT_COMPRESS,NullS);
  if (opt_connect_timeout)
  {
    uint tmp=opt_connect_timeout;
    mysql_options(&mysql,MYSQL_OPT_CONNECT_TIMEOUT, (char*) &tmp);
  }
#ifdef HAVE_OPENSSL
  if (opt_use_ssl)
  {
    mysql_ssl_set(&mysql, opt_ssl_key, opt_ssl_cert, opt_ssl_ca,
		  opt_ssl_capath, opt_ssl_cipher);
    mysql_options(&mysql, MYSQL_OPT_SSL_CRL, opt_ssl_crl);
    mysql_options(&mysql, MYSQL_OPT_SSL_CRLPATH, opt_ssl_crlpath);
  }
  mysql_options(&mysql,MYSQL_OPT_SSL_VERIFY_SERVER_CERT,
                (char*)&opt_ssl_verify_server_cert);
#endif
  if (opt_protocol)
    mysql_options(&mysql,MYSQL_OPT_PROTOCOL,(char*)&opt_protocol);
#if defined (_WIN32) && !defined (EMBEDDED_LIBRARY)
  if (shared_memory_base_name)
    mysql_options(&mysql,MYSQL_SHARED_MEMORY_BASE_NAME,shared_memory_base_name);
#endif
  mysql_options(&mysql, MYSQL_SET_CHARSET_NAME, default_charset);
  error_flags= (myf)(opt_nobeep ? 0 : ME_BELL);

  if (opt_plugin_dir && *opt_plugin_dir)
    mysql_options(&mysql, MYSQL_PLUGIN_DIR, opt_plugin_dir);

  if (opt_default_auth && *opt_default_auth)
    mysql_options(&mysql, MYSQL_DEFAULT_AUTH, opt_default_auth);

  mysql_options(&mysql, MYSQL_OPT_CONNECT_ATTR_RESET, 0);
  mysql_options4(&mysql, MYSQL_OPT_CONNECT_ATTR_ADD,
                 "program_name", "mysqladmin");
  if (using_opt_enable_cleartext_plugin)
    mysql_options(&mysql, MYSQL_ENABLE_CLEARTEXT_PLUGIN, 
                  (char*) &opt_enable_cleartext_plugin);

  first_command= find_type(argv[0], &command_typelib, FIND_TYPE_BASIC);
  can_handle_passwords= 
    (first_command == ADMIN_PASSWORD || first_command == ADMIN_OLD_PASSWORD) ?
    TRUE : FALSE;
  mysql_options(&mysql, MYSQL_OPT_CAN_HANDLE_EXPIRED_PASSWORDS,
                &can_handle_passwords);

  if (sql_connect(&mysql, option_wait))
  {
    /*
      We couldn't get an initial connection and will definitely exit.
      The following just determines the exit-code we'll give.
    */

    unsigned int err= mysql_errno(&mysql);
    if (err >= CR_MIN_ERROR && err <= CR_MAX_ERROR)
      error= 1;
    else
    {
      /* Return 0 if all commands are PING */
      for (; argc > 0; argv++, argc--)
      {
        if (find_type(argv[0], &command_typelib, FIND_TYPE_BASIC) !=
            ADMIN_PING)
        {
          error= 1;
          break;
        }
      }
    }
  }
  else
  {
    /*
      --count=0 aborts right here. Otherwise iff --sleep=t ("interval")
      is given a t!=0, we get an endless loop, or n iterations if --count=n
      was given an n!=0. If --sleep wasn't given, we get one iteration.

      To wit, --wait loops the connection-attempts, while --sleep loops
      the command execution (endlessly if no --count is given).
    */

    while (!interrupted && (!opt_count_iterations || nr_iterations))
    {
      new_line = 0;

      if ((error= execute_commands(&mysql,argc,commands)))
      {
        /*
          Unknown/malformed command always aborts and can't be --forced.
          If the user got confused about the syntax, proceeding would be
          dangerous ...
        */
	if (error > 0)
	  break;

        /*
          Command was well-formed, but failed on the server. Might succeed
          on retry (if conditions on server change etc.), but needs --force
          to retry.
        */
        if (!option_force)
          break;
      }                                         /* if((error= ... */

      if (interval)                             /* --sleep=interval given */
      {
        if (opt_count_iterations && --nr_iterations == 0)
          break;

        /*
          If connection was dropped (unintentionally, or due to SHUTDOWN),
          re-establish it if --wait ("retry-connect") was given and user
          didn't signal for us to die. Otherwise, signal failure.
        */

	if (mysql.net.vio == 0)
	{
	  if (option_wait && !interrupted)
	  {
	    sleep(1);
	    sql_connect(&mysql, option_wait);
	    /*
	      continue normally and decrease counters so that
	      "mysqladmin --count=1 --wait=1 shutdown"
	      cannot loop endlessly.
	    */
	  }
	  else
	  {
	    /*
	      connexion broke, and we have no order to re-establish it. fail.
	    */
	    if (!option_force)
	      error= 1;
	    break;
	  }
	}                                       /* lost connection */

	sleep(interval);
	if (new_line)
	  puts("");
      }
      else
        break;                                  /* no --sleep, done looping */
    }                                           /* command-loop */
  }                                             /* got connection */

  mysql_close(&mysql);
  my_free(opt_password);
  my_free(user);
#if defined (_WIN32) && !defined (EMBEDDED_LIBRARY)
  my_free(shared_memory_base_name);
#endif
  free_defaults(save_argv);
  my_end(my_end_arg);
  exit(error ? 1 : 0);
  return 0;
}