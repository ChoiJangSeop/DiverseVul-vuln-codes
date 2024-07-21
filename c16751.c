static int init_common_variables()
{
  umask(((~my_umask) & 0666));
  my_decimal_set_zero(&decimal_zero); // set decimal_zero constant;

  tzset();			// Set tzname

  sf_leaking_memory= 0; // no memory leaks from now on

  max_system_variables.pseudo_thread_id= (ulong)~0;
  server_start_time= flush_status_time= my_time(0);

  rpl_filter= new Rpl_filter;
  binlog_filter= new Rpl_filter;
  if (!rpl_filter || !binlog_filter)
  {
    sql_perror("Could not allocate replication and binlog filters");
    return 1;
  }

  if (init_thread_environment() ||
      mysql_init_variables())
    return 1;

  if (ignore_db_dirs_init())
    return 1;

#ifdef HAVE_TZNAME
  struct tm tm_tmp;
  localtime_r(&server_start_time,&tm_tmp);
  const char *tz_name=  tzname[tm_tmp.tm_isdst != 0 ? 1 : 0];
#ifdef _WIN32
  /*
    Time zone name may be localized and contain non-ASCII characters,
    Convert from ANSI encoding to UTF8.
  */
  wchar_t wtz_name[sizeof(system_time_zone)];
  mbstowcs(wtz_name, tz_name, sizeof(system_time_zone)-1);
  WideCharToMultiByte(CP_UTF8,0, wtz_name, -1, system_time_zone, 
    sizeof(system_time_zone) - 1, NULL, NULL);
#else
  strmake_buf(system_time_zone, tz_name);
#endif /* _WIN32 */
#endif /* HAVE_TZNAME */

  /*
    We set SYSTEM time zone as reasonable default and
    also for failure of my_tz_init() and bootstrap mode.
    If user explicitly set time zone with --default-time-zone
    option we will change this value in my_tz_init().
  */
  global_system_variables.time_zone= my_tz_SYSTEM;

#ifdef HAVE_PSI_INTERFACE
  /*
    Complete the mysql_bin_log initialization.
    Instrumentation keys are known only after the performance schema initialization,
    and can not be set in the MYSQL_BIN_LOG constructor (called before main()).
  */
  mysql_bin_log.set_psi_keys(key_BINLOG_LOCK_index,
                             key_BINLOG_update_cond,
                             key_file_binlog,
                             key_file_binlog_index,
                             key_BINLOG_COND_queue_busy);
#endif

  /*
    Init mutexes for the global MYSQL_BIN_LOG objects.
    As safe_mutex depends on what MY_INIT() does, we can't init the mutexes of
    global MYSQL_BIN_LOGs in their constructors, because then they would be
    inited before MY_INIT(). So we do it here.
  */
  mysql_bin_log.init_pthread_objects();

  /* TODO: remove this when my_time_t is 64 bit compatible */
  if (!IS_TIME_T_VALID_FOR_TIMESTAMP(server_start_time))
  {
    sql_print_error("This MySQL server doesn't support dates later then 2038");
    return 1;
  }

  if (gethostname(glob_hostname,sizeof(glob_hostname)) < 0)
  {
    /*
      Get hostname of computer (used by 'show variables') and as default
      basename for the pid file if --log-basename is not given.
    */
    strmake(glob_hostname, STRING_WITH_LEN("localhost"));
    sql_print_warning("gethostname failed, using '%s' as hostname",
                        glob_hostname);
    opt_log_basename= const_cast<char *>("mysql");
  }
  else
    opt_log_basename= glob_hostname;

  if (!*pidfile_name)
  {
    strmake(pidfile_name, opt_log_basename, sizeof(pidfile_name)-5);
    strmov(fn_ext(pidfile_name),".pid");		// Add proper extension
  }

  /*
    The default-storage-engine entry in my_long_options should have a
    non-null default value. It was earlier intialized as
    (longlong)"MyISAM" in my_long_options but this triggered a
    compiler error in the Sun Studio 12 compiler. As a work-around we
    set the def_value member to 0 in my_long_options and initialize it
    to the correct value here.

    From MySQL 5.5 onwards, the default storage engine is InnoDB
    (except in the embedded server, where the default continues to
    be MyISAM)
  */
#if defined(WITH_INNOBASE_STORAGE_ENGINE) || defined(WITH_XTRADB_STORAGE_ENGINE)
  default_storage_engine= const_cast<char *>("InnoDB");
#else
  default_storage_engine= const_cast<char *>("MyISAM");
#endif

  /*
    Add server status variables to the dynamic list of
    status variables that is shown by SHOW STATUS.
    Later, in plugin_init, and mysql_install_plugin
    new entries could be added to that list.
  */
  if (add_status_vars(status_vars))
    return 1; // an error was already reported

#ifndef DBUG_OFF
  /*
    We have few debug-only commands in com_status_vars, only visible in debug
    builds. for simplicity we enable the assert only in debug builds

    There are 8 Com_ variables which don't have corresponding SQLCOM_ values:
    (TODO strictly speaking they shouldn't be here, should not have Com_ prefix
    that is. Perhaps Stmt_ ? Comstmt_ ? Prepstmt_ ?)

      Com_admin_commands       => com_other
      Com_stmt_close           => com_stmt_close
      Com_stmt_execute         => com_stmt_execute
      Com_stmt_fetch           => com_stmt_fetch
      Com_stmt_prepare         => com_stmt_prepare
      Com_stmt_reprepare       => com_stmt_reprepare
      Com_stmt_reset           => com_stmt_reset
      Com_stmt_send_long_data  => com_stmt_send_long_data

    With this correction the number of Com_ variables (number of elements in
    the array, excluding the last element - terminator) must match the number
    of SQLCOM_ constants.
  */
  compile_time_assert(sizeof(com_status_vars)/sizeof(com_status_vars[0]) - 1 ==
                     SQLCOM_END + 8);
#endif

  if (get_options(&remaining_argc, &remaining_argv))
    return 1;
  set_server_version();

  if (!opt_help)
    sql_print_information("%s (mysqld %s) starting as process %lu ...",
                          my_progname, server_version, (ulong) getpid());

#ifndef EMBEDDED_LIBRARY
  if (opt_abort && !opt_verbose)
    unireg_abort(0);
#endif /*!EMBEDDED_LIBRARY*/

  DBUG_PRINT("info",("%s  Ver %s for %s on %s\n",my_progname,
		     server_version, SYSTEM_TYPE,MACHINE_TYPE));

#ifdef HAVE_LARGE_PAGES
  /* Initialize large page size */
  if (opt_large_pages && (opt_large_page_size= my_get_large_page_size()))
  {
      DBUG_PRINT("info", ("Large page set, large_page_size = %d",
                 opt_large_page_size));
      my_use_large_pages= 1;
      my_large_page_size= opt_large_page_size;
  }
  else
  {
    opt_large_pages= 0;
    /* 
       Either not configured to use large pages or Linux haven't
       been compiled with large page support
    */
  }
#endif /* HAVE_LARGE_PAGES */
#ifdef HAVE_SOLARIS_LARGE_PAGES
#define LARGE_PAGESIZE (4*1024*1024)  /* 4MB */
#define SUPER_LARGE_PAGESIZE (256*1024*1024)  /* 256MB */
  if (opt_large_pages)
  {
  /*
    tell the kernel that we want to use 4/256MB page for heap storage
    and also for the stack. We use 4 MByte as default and if the
    super-large-page is set we increase it to 256 MByte. 256 MByte
    is for server installations with GBytes of RAM memory where
    the MySQL Server will have page caches and other memory regions
    measured in a number of GBytes.
    We use as big pages as possible which isn't bigger than the above
    desired page sizes.
  */
   int nelem;
   size_t max_desired_page_size;
   if (opt_super_large_pages)
     max_desired_page_size= SUPER_LARGE_PAGESIZE;
   else
     max_desired_page_size= LARGE_PAGESIZE;
   nelem = getpagesizes(NULL, 0);
   if (nelem > 0)
   {
     size_t *pagesize = (size_t *) malloc(sizeof(size_t) * nelem);
     if (pagesize != NULL && getpagesizes(pagesize, nelem) > 0)
     {
       size_t max_page_size= 0;
       for (int i= 0; i < nelem; i++)
       {
         if (pagesize[i] > max_page_size &&
             pagesize[i] <= max_desired_page_size)
            max_page_size= pagesize[i];
       }
       free(pagesize);
       if (max_page_size > 0)
       {
         struct memcntl_mha mpss;

         mpss.mha_cmd= MHA_MAPSIZE_BSSBRK;
         mpss.mha_pagesize= max_page_size;
         mpss.mha_flags= 0;
         memcntl(NULL, 0, MC_HAT_ADVISE, (caddr_t)&mpss, 0, 0);
         mpss.mha_cmd= MHA_MAPSIZE_STACK;
         memcntl(NULL, 0, MC_HAT_ADVISE, (caddr_t)&mpss, 0, 0);
       }
     }
   }
  }
#endif /* HAVE_SOLARIS_LARGE_PAGES */

  /* connections and databases needs lots of files */
  {
    uint files, wanted_files, max_open_files;

    /* MyISAM requires two file handles per table. */
    wanted_files= (10 + max_connections + extra_max_connections +
                   table_cache_size*2);
    /*
      We are trying to allocate no less than max_connections*5 file
      handles (i.e. we are trying to set the limit so that they will
      be available).  In addition, we allocate no less than how much
      was already allocated.  However below we report a warning and
      recompute values only if we got less file handles than were
      explicitly requested.  No warning and re-computation occur if we
      can't get max_connections*5 but still got no less than was
      requested (value of wanted_files).
    */
    max_open_files= max(max(wanted_files,
                            (max_connections + extra_max_connections)*5),
                        open_files_limit);
    files= my_set_max_open_files(max_open_files);

    if (files < wanted_files)
    {
      if (!open_files_limit)
      {
        /*
          If we have requested too much file handles than we bring
          max_connections in supported bounds.
        */
        max_connections= (ulong) min(files-10-TABLE_OPEN_CACHE_MIN*2,
                                     max_connections);
        /*
          Decrease table_cache_size according to max_connections, but
          not below TABLE_OPEN_CACHE_MIN.  Outer min() ensures that we
          never increase table_cache_size automatically (that could
          happen if max_connections is decreased above).
        */
        table_cache_size= (ulong) min(max((files-10-max_connections)/2,
                                          TABLE_OPEN_CACHE_MIN),
                                      table_cache_size);
	DBUG_PRINT("warning",
		   ("Changed limits: max_open_files: %u  max_connections: %ld  table_cache: %ld",
		    files, max_connections, table_cache_size));
	if (global_system_variables.log_warnings)
	  sql_print_warning("Changed limits: max_open_files: %u  max_connections: %ld  table_cache: %ld",
			files, max_connections, table_cache_size);
      }
      else if (global_system_variables.log_warnings)
	sql_print_warning("Could not increase number of max_open_files to more than %u (request: %u)", files, wanted_files);
    }
    open_files_limit= files;
  }
  unireg_init(opt_specialflag); /* Set up extern variabels */
  if (!(my_default_lc_messages=
        my_locale_by_name(lc_messages)))
  {
    sql_print_error("Unknown locale: '%s'", lc_messages);
    return 1;
  }
  global_system_variables.lc_messages= my_default_lc_messages;
  if (init_errmessage())	/* Read error messages from file */
    return 1;
  init_client_errs();
  mysql_library_init(unused,unused,unused); /* for replication */
  lex_init();
  if (item_create_init())
    return 1;
  item_init();
#ifndef EMBEDDED_LIBRARY
  my_regex_init(&my_charset_latin1, check_enough_stack_size);
  my_string_stack_guard= check_enough_stack_size;
#else
  my_regex_init(&my_charset_latin1, NULL);
#endif
  /*
    Process a comma-separated character set list and choose
    the first available character set. This is mostly for
    test purposes, to be able to start "mysqld" even if
    the requested character set is not available (see bug#18743).
  */
  for (;;)
  {
    char *next_character_set_name= strchr(default_character_set_name, ',');
    if (next_character_set_name)
      *next_character_set_name++= '\0';
    if (!(default_charset_info=
          get_charset_by_csname(default_character_set_name,
                                MY_CS_PRIMARY, MYF(MY_WME))))
    {
      if (next_character_set_name)
      {
        default_character_set_name= next_character_set_name;
        default_collation_name= 0;          // Ignore collation
      }
      else
        return 1;                           // Eof of the list
    }
    else
      break;
  }

  if (default_collation_name)
  {
    CHARSET_INFO *default_collation;
    default_collation= get_charset_by_name(default_collation_name, MYF(0));
    if (!default_collation)
    {
      sql_print_error(ER_DEFAULT(ER_UNKNOWN_COLLATION), default_collation_name);
      return 1;
    }
    if (!my_charset_same(default_charset_info, default_collation))
    {
      sql_print_error(ER_DEFAULT(ER_COLLATION_CHARSET_MISMATCH),
		      default_collation_name,
		      default_charset_info->csname);
      return 1;
    }
    default_charset_info= default_collation;
  }
  /* Set collactions that depends on the default collation */
  global_system_variables.collation_server=	 default_charset_info;
  global_system_variables.collation_database=	 default_charset_info;
  global_system_variables.collation_connection=  default_charset_info;
  global_system_variables.character_set_results= default_charset_info;
  global_system_variables.character_set_client=  default_charset_info;

  if (!(character_set_filesystem=
        get_charset_by_csname(character_set_filesystem_name,
                              MY_CS_PRIMARY, MYF(MY_WME))))
    return 1;
  global_system_variables.character_set_filesystem= character_set_filesystem;

  if (!(my_default_lc_time_names=
        my_locale_by_name(lc_time_names_name)))
  {
    sql_print_error("Unknown locale: '%s'", lc_time_names_name);
    return 1;
  }
  global_system_variables.lc_time_names= my_default_lc_time_names;

  /* check log options and issue warnings if needed */
  if (opt_log && opt_logname && *opt_logname &&
      !(log_output_options & (LOG_FILE | LOG_NONE)))
    sql_print_warning("Although a path was specified for the "
                      "--log option, log tables are used. "
                      "To enable logging to files use the --log-output option.");

  if (opt_slow_log && opt_slow_logname && *opt_slow_logname &&
      !(log_output_options & (LOG_FILE | LOG_NONE)))
    sql_print_warning("Although a path was specified for the "
                      "--log-slow-queries option, log tables are used. "
                      "To enable logging to files use the --log-output=file option.");

  if (!opt_logname || !*opt_logname)
    make_default_log_name(&opt_logname, ".log", false);
  if (!opt_slow_logname || !*opt_slow_logname)
    make_default_log_name(&opt_slow_logname, "-slow.log", false);

#if defined(ENABLED_DEBUG_SYNC)
  /* Initialize the debug sync facility. See debug_sync.cc. */
  if (debug_sync_init())
    return 1; /* purecov: tested */
#endif /* defined(ENABLED_DEBUG_SYNC) */

#if (ENABLE_TEMP_POOL)
  if (use_temp_pool && bitmap_init(&temp_pool,0,1024,1))
    return 1;
#else
  use_temp_pool= 0;
#endif

  if (my_dboptions_cache_init())
    return 1;

  /*
    Ensure that lower_case_table_names is set on system where we have case
    insensitive names.  If this is not done the users MyISAM tables will
    get corrupted if accesses with names of different case.
  */
  DBUG_PRINT("info", ("lower_case_table_names: %d", lower_case_table_names));
  lower_case_file_system= test_if_case_insensitive(mysql_real_data_home);
  if (!lower_case_table_names && lower_case_file_system == 1)
  {
    if (lower_case_table_names_used)
    {
#if MYSQL_VERSION_ID < 100100
      if (global_system_variables.log_warnings)
        sql_print_warning("You have forced lower_case_table_names to 0 through "
                          "a command-line option, even though your file system "
                          "'%s' is case insensitive.  This means that you can "
                          "corrupt your tables if you access them using names "
                          "with different letter case. You should consider "
                          "changing lower_case_table_names to 1 or 2",
                          mysql_real_data_home);
#else
      sql_print_error("The server option 'lower_case_table_names' is "
                      "configured to use case sensitive table names but the "
                      "data directory resides on a case-insensitive file system. "
                      "Please use a case sensitive file system for your data "
                      "directory or switch to a case-insensitive table name "
                      "mode.");
#endif
      return 1;
    }
    else
    {
      if (global_system_variables.log_warnings)
	sql_print_warning("Setting lower_case_table_names=2 because file system for %s is case insensitive", mysql_real_data_home);
      lower_case_table_names= 2;
    }
  }
  else if (lower_case_table_names == 2 &&
           !(lower_case_file_system= (lower_case_file_system == 1)))
  {
    if (global_system_variables.log_warnings)
      sql_print_warning("lower_case_table_names was set to 2, even though your "
                        "the file system '%s' is case sensitive.  Now setting "
                        "lower_case_table_names to 0 to avoid future problems.",
			mysql_real_data_home);
    lower_case_table_names= 0;
  }
  else
  {
    lower_case_file_system= (lower_case_file_system == 1);
  }

  /* Reset table_alias_charset, now that lower_case_table_names is set. */
  table_alias_charset= (lower_case_table_names ?
			files_charset_info :
			&my_charset_bin);

  if (ignore_db_dirs_process_additions())
  {
    sql_print_error("An error occurred while storing ignore_db_dirs to a hash.");
    return 1;
  }

  return 0;
}