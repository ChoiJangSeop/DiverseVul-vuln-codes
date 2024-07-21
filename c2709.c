int main(int argc, char **argv)
{
  struct st_command *command;
  my_bool q_send_flag= 0, abort_flag= 0;
  uint command_executed= 0, last_command_executed= 0;
  char save_file[FN_REFLEN];
  char output_file[FN_REFLEN];
  MY_INIT(argv[0]);

  save_file[0]= 0;
  output_file[0]= 0;
  TMPDIR[0]= 0;

  init_signal_handling();

  /* Init expected errors */
  memset(&saved_expected_errors, 0, sizeof(saved_expected_errors));

#ifdef EMBEDDED_LIBRARY
  /* set appropriate stack for the 'query' threads */
  (void) pthread_attr_init(&cn_thd_attrib);
  pthread_attr_setstacksize(&cn_thd_attrib, DEFAULT_THREAD_STACK);
#endif /*EMBEDDED_LIBRARY*/

  /* Init file stack */
  memset(file_stack, 0, sizeof(file_stack));
  file_stack_end=
    file_stack + (sizeof(file_stack)/sizeof(struct st_test_file)) - 1;
  cur_file= file_stack;

  /* Init block stack */
  memset(block_stack, 0, sizeof(block_stack));
  block_stack_end=
    block_stack + (sizeof(block_stack)/sizeof(struct st_block)) - 1;
  cur_block= block_stack;
  cur_block->ok= TRUE; /* Outer block should always be executed */
  cur_block->cmd= cmd_none;

  my_init_dynamic_array(&q_lines, sizeof(struct st_command*), 1024, 1024);

  if (my_hash_init(&var_hash, charset_info,
                   1024, 0, 0, get_var_key, var_free, MYF(0)))
    die("Variable hash initialization failed");

  {
    char path_separator[]= { FN_LIBCHAR, 0 };
    var_set_string("SYSTEM_PATH_SEPARATOR", path_separator);
  }
  var_set_string("MYSQL_SERVER_VERSION", MYSQL_SERVER_VERSION);
  var_set_string("MYSQL_SYSTEM_TYPE", SYSTEM_TYPE);
  var_set_string("MYSQL_MACHINE_TYPE", MACHINE_TYPE);
  if (sizeof(void *) == 8) {
    var_set_string("MYSQL_SYSTEM_ARCHITECTURE", "64");
  } else {
    var_set_string("MYSQL_SYSTEM_ARCHITECTURE", "32");
  }

  memset(&master_pos, 0, sizeof(master_pos));

  parser.current_line= parser.read_lines= 0;
  memset(&var_reg, 0, sizeof(var_reg));

  init_builtin_echo();
#ifdef _WIN32
  is_windows= 1;
  init_win_path_patterns();
#endif

  init_dynamic_string(&ds_res, "", 2048, 2048);

  parse_args(argc, argv);

  log_file.open(opt_logdir, result_file_name, ".log");
  verbose_msg("Logging to '%s'.", log_file.file_name());
  if (opt_mark_progress)
  {
    progress_file.open(opt_logdir, result_file_name, ".progress");
    verbose_msg("Tracing progress in '%s'.", progress_file.file_name());
  }

  /* Init connections, allocate 1 extra as buffer + 1 for default */
  connections= (struct st_connection*)
    my_malloc(PSI_NOT_INSTRUMENTED,
              (opt_max_connections+2) * sizeof(struct st_connection),
              MYF(MY_WME | MY_ZEROFILL));
  connections_end= connections + opt_max_connections +1;
  next_con= connections + 1;
  
  var_set_int("$PS_PROTOCOL", ps_protocol);
  var_set_int("$SP_PROTOCOL", sp_protocol);
  var_set_int("$VIEW_PROTOCOL", view_protocol);
  var_set_int("$OPT_TRACE_PROTOCOL", opt_trace_protocol);
  var_set_int("$EXPLAIN_PROTOCOL", explain_protocol);
  var_set_int("$JSON_EXPLAIN_PROTOCOL", json_explain_protocol);
  var_set_int("$CURSOR_PROTOCOL", cursor_protocol);

  var_set_int("$ENABLED_QUERY_LOG", 1);
  var_set_int("$ENABLED_ABORT_ON_ERROR", 1);
  var_set_int("$ENABLED_RESULT_LOG", 1);
  var_set_int("$ENABLED_CONNECT_LOG", 0);
  var_set_int("$ENABLED_WARNINGS", 1);
  var_set_int("$ENABLED_INFO", 0);
  var_set_int("$ENABLED_METADATA", 0);

  DBUG_PRINT("info",("result_file: '%s'",
                     result_file_name ? result_file_name : ""));
  verbose_msg("Results saved in '%s'.", 
              result_file_name ? result_file_name : "");
  if (mysql_server_init(embedded_server_arg_count,
			embedded_server_args,
			(char**) embedded_server_groups))
    die("Can't initialize MySQL server");
  server_initialized= 1;
  if (cur_file == file_stack && cur_file->file == 0)
  {
    cur_file->file= stdin;
    cur_file->file_name= my_strdup(PSI_NOT_INSTRUMENTED,
                                   "<stdin>", MYF(MY_WME));
    cur_file->lineno= 1;
  }
  var_set_string("MYSQLTEST_FILE", cur_file->file_name);
  init_re();

  /* Cursor protcol implies ps protocol */
  if (cursor_protocol)
    ps_protocol= 1;

  ps_protocol_enabled= ps_protocol;
  sp_protocol_enabled= sp_protocol;
  view_protocol_enabled= view_protocol;
  opt_trace_protocol_enabled= opt_trace_protocol;
  explain_protocol_enabled= explain_protocol;
  json_explain_protocol_enabled= json_explain_protocol;
  cursor_protocol_enabled= cursor_protocol;

  st_connection *con= connections;
#ifdef EMBEDDED_LIBRARY
  if (ps_protocol)
    die("--ps-protocol is not supported in embedded mode");
  init_connection_thd(con);
#endif /*EMBEDDED_LIBRARY*/
  if (!( mysql_init(&con->mysql)))
    die("Failed in mysql_init()");
  if (opt_connect_timeout)
    mysql_options(&con->mysql, MYSQL_OPT_CONNECT_TIMEOUT,
                  (void *) &opt_connect_timeout);
  if (opt_compress)
    mysql_options(&con->mysql,MYSQL_OPT_COMPRESS,NullS);
  mysql_options(&con->mysql, MYSQL_OPT_LOCAL_INFILE, 0);
  mysql_options(&con->mysql, MYSQL_SET_CHARSET_NAME,
                charset_info->csname);
  if (opt_charsets_dir)
    mysql_options(&con->mysql, MYSQL_SET_CHARSET_DIR,
                  opt_charsets_dir);

#ifndef EMBEDDED_LIBRARY
  if (opt_protocol)
    mysql_options(&con->mysql,MYSQL_OPT_PROTOCOL,(char*)&opt_protocol);
#endif

#if defined(HAVE_OPENSSL) && !defined(EMBEDDED_LIBRARY)

  if (opt_use_ssl)
  {
    mysql_ssl_set(&con->mysql, opt_ssl_key, opt_ssl_cert, opt_ssl_ca,
		  opt_ssl_capath, opt_ssl_cipher);
    mysql_options(&con->mysql, MYSQL_OPT_SSL_CRL, opt_ssl_crl);
    mysql_options(&con->mysql, MYSQL_OPT_SSL_CRLPATH, opt_ssl_crlpath);
#if MYSQL_VERSION_ID >= 50000
    /* Turn on ssl_verify_server_cert only if host is "localhost" */
    opt_ssl_verify_server_cert= opt_host && !strcmp(opt_host, "localhost");
    mysql_options(&con->mysql, MYSQL_OPT_SSL_VERIFY_SERVER_CERT,
                  &opt_ssl_verify_server_cert);
#endif
  }
#endif

#if defined (_WIN32) && !defined (EMBEDDED_LIBRARY)
  if (shared_memory_base_name)
    mysql_options(&con->mysql,MYSQL_SHARED_MEMORY_BASE_NAME,shared_memory_base_name);
#endif

  if (!(con->name = my_strdup(PSI_NOT_INSTRUMENTED,
                              "default", MYF(MY_WME))))
    die("Out of memory");

  safe_connect(&con->mysql, con->name, opt_host, opt_user, opt_pass,
               opt_db, opt_port, unix_sock);

  /* Use all time until exit if no explicit 'start_timer' */
  timer_start= timer_now();

  /*
    Initialize $mysql_errno with -1, so we can
    - distinguish it from valid values ( >= 0 ) and
    - detect if there was never a command sent to the server
  */
  var_set_errno(-1);

  set_current_connection(con);

  if (opt_include)
  {
    open_file(opt_include);
  }

  verbose_msg("Start processing test commands from '%s' ...", cur_file->file_name);
  while (!read_command(&command) && !abort_flag)
  {
    int current_line_inc = 1, processed = 0;
    if (command->type == Q_UNKNOWN || command->type == Q_COMMENT_WITH_COMMAND)
      get_command_type(command);

    if (parsing_disabled &&
        command->type != Q_ENABLE_PARSING &&
        command->type != Q_DISABLE_PARSING)
    {
      /* Parsing is disabled, silently convert this line to a comment */
      command->type= Q_COMMENT;
    }

    /* (Re-)set abort_on_error for this command */
    command->abort_on_error= (command->expected_errors.count == 0 &&
                              abort_on_error);
    
    /* delimiter needs to be executed so we can continue to parse */
    my_bool ok_to_do= cur_block->ok || command->type == Q_DELIMITER;
    /*
      Some commands need to be "done" the first time if they may get
      re-iterated over in a true context. This can only happen if there's 
      a while loop at some level above the current block.
    */
    if (!ok_to_do)
    {
      if (command->type == Q_SOURCE ||
          command->type == Q_ERROR ||
          command->type == Q_WRITE_FILE ||
          command->type == Q_APPEND_FILE ||
	  command->type == Q_PERL)
      {
	for (struct st_block *stb= cur_block-1; stb >= block_stack; stb--)
	{
	  if (stb->cmd == cmd_while)
	  {
	    ok_to_do= 1;
	    break;
	  }
	}
      }
    }

    if (ok_to_do)
    {
      command->last_argument= command->first_argument;
      processed = 1;
      /* Need to remember this for handle_error() */
      curr_command= command;
      switch (command->type) {
      case Q_CONNECT:
        do_connect(command);
        break;
      case Q_CONNECTION: select_connection(command); break;
      case Q_DISCONNECT:
      case Q_DIRTY_CLOSE:
	do_close_connection(command); break;
      case Q_ENABLE_QUERY_LOG:
        set_property(command, P_QUERY, 0);
        break;
      case Q_DISABLE_QUERY_LOG:
        set_property(command, P_QUERY, 1);
        break;
      case Q_ENABLE_ABORT_ON_ERROR:
        set_property(command, P_ABORT, 1);
        break;
      case Q_DISABLE_ABORT_ON_ERROR:
        set_property(command, P_ABORT, 0);
        break;
      case Q_ENABLE_RESULT_LOG:
        set_property(command, P_RESULT, 0);
        break;
      case Q_DISABLE_RESULT_LOG:
        set_property(command, P_RESULT, 1);
        break;
      case Q_ENABLE_CONNECT_LOG:
        set_property(command, P_CONNECT, 0);
        break;
      case Q_DISABLE_CONNECT_LOG:
        set_property(command, P_CONNECT, 1);
        break;
      case Q_ENABLE_WARNINGS:
        set_property(command, P_WARN, 0);
        break;
      case Q_DISABLE_WARNINGS:
        set_property(command, P_WARN, 1);
        break;
      case Q_ENABLE_INFO:
        set_property(command, P_INFO, 0);
        break;
      case Q_DISABLE_INFO:
        set_property(command, P_INFO, 1);
        break;
      case Q_ENABLE_METADATA:
        set_property(command, P_META, 1);
        break;
      case Q_DISABLE_METADATA:
        set_property(command, P_META, 0);
        break;
      case Q_SOURCE: do_source(command); break;
      case Q_SLEEP: do_sleep(command, 0); break;
      case Q_REAL_SLEEP: do_sleep(command, 1); break;
      case Q_WAIT_FOR_SLAVE_TO_STOP: do_wait_for_slave_to_stop(command); break;
      case Q_INC: do_modify_var(command, DO_INC); break;
      case Q_DEC: do_modify_var(command, DO_DEC); break;
      case Q_ECHO: do_echo(command); command_executed++; break;
      case Q_SYSTEM:
        die("'system' command  is deprecated, use exec or\n"\
            "  see the manual for portable commands to use");
	break;
      case Q_REMOVE_FILE: do_remove_file(command); break;
      case Q_REMOVE_FILES_WILDCARD: do_remove_files_wildcard(command); break;
      case Q_MKDIR: do_mkdir(command); break;
      case Q_RMDIR: do_rmdir(command); break;
      case Q_LIST_FILES: do_list_files(command); break;
      case Q_LIST_FILES_WRITE_FILE:
        do_list_files_write_file_command(command, FALSE);
        break;
      case Q_LIST_FILES_APPEND_FILE:
        do_list_files_write_file_command(command, TRUE);
        break;
      case Q_FILE_EXIST: do_file_exist(command); break;
      case Q_WRITE_FILE: do_write_file(command); break;
      case Q_APPEND_FILE: do_append_file(command); break;
      case Q_DIFF_FILES: do_diff_files(command); break;
      case Q_SEND_QUIT: do_send_quit(command); break;
      case Q_CHANGE_USER: do_change_user(command); break;
      case Q_CAT_FILE: do_cat_file(command); break;
      case Q_COPY_FILE: do_copy_file(command); break;
      case Q_MOVE_FILE: do_move_file(command); break;
      case Q_CHMOD_FILE: do_chmod_file(command); break;
      case Q_PERL: do_perl(command); break;
      case Q_RESULT_FORMAT_VERSION: do_result_format_version(command); break;
      case Q_DELIMITER:
        do_delimiter(command);
	break;
      case Q_DISPLAY_VERTICAL_RESULTS:
        display_result_vertically= TRUE;
        break;
      case Q_DISPLAY_HORIZONTAL_RESULTS:
	display_result_vertically= FALSE;
        break;
      case Q_SORTED_RESULT:
        /*
          Turn on sorting of result set, will be reset after next
          command
        */
	display_result_sorted= TRUE;
        break;
      case Q_LOWERCASE:
        /*
          Turn on lowercasing of result, will be reset after next
          command
        */
        display_result_lower= TRUE;
        break;
      case Q_LET: do_let(command); break;
      case Q_EVAL_RESULT:
        die("'eval_result' command  is deprecated");
      case Q_EVAL:
      case Q_QUERY_VERTICAL:
      case Q_QUERY_HORIZONTAL:
	if (command->query == command->query_buf)
        {
          /* Skip the first part of command, i.e query_xxx */
	  command->query= command->first_argument;
          command->first_word_len= 0;
        }
	/* fall through */
      case Q_QUERY:
      case Q_REAP:
      {
	my_bool old_display_result_vertically= display_result_vertically;
        /* Default is full query, both reap and send  */
        int flags= QUERY_REAP_FLAG | QUERY_SEND_FLAG;

        if (q_send_flag)
        {
          /* Last command was an empty 'send' */
          flags= QUERY_SEND_FLAG;
          q_send_flag= 0;
        }
        else if (command->type == Q_REAP)
        {
          flags= QUERY_REAP_FLAG;
        }

        /* Check for special property for this query */
        display_result_vertically|= (command->type == Q_QUERY_VERTICAL);

        /*
          We run EXPLAIN _before_ the query. If query is UPDATE/DELETE is
          matters: a DELETE may delete rows, and then EXPLAIN DELETE will
          usually terminate quickly with "no matching rows". To make it more
          interesting, EXPLAIN is now first.
        */
        if (explain_protocol_enabled)
          run_explain(cur_con, command, flags, 0);
        if (json_explain_protocol_enabled)
          run_explain(cur_con, command, flags, 1);
	/* Check for 'require' */
	if (*save_file)
	{
	  strmake(command->require_file, save_file, sizeof(save_file) - 1);
	  *save_file= 0;
	}
	if (*output_file)
	{
	  strmake(command->output_file, output_file, sizeof(output_file) - 1);
	  *output_file= 0;
	}
	run_query(cur_con, command, flags);
	display_opt_trace(cur_con, command, flags);
	command_executed++;
        command->last_argument= command->end;

        /* Restore settings */
	display_result_vertically= old_display_result_vertically;

	break;
      }
      case Q_SEND:
      case Q_SEND_EVAL:
        if (!*command->first_argument)
        {
          /*
            This is a send without arguments, it indicates that _next_ query
            should be send only
          */
          q_send_flag= 1;
          break;
        }

        /* Remove "send" if this is first iteration */
	if (command->query == command->query_buf)
	  command->query= command->first_argument;

	/*
	  run_query() can execute a query partially, depending on the flags.
	  QUERY_SEND_FLAG flag without QUERY_REAP_FLAG tells it to just send
          the query and read the result some time later when reap instruction
	  is given on this connection.
        */
	run_query(cur_con, command, QUERY_SEND_FLAG);
	command_executed++;
        command->last_argument= command->end;
	break;
      case Q_REQUIRE:
	do_get_file_name(command, save_file, sizeof(save_file));
	break;
      case Q_ERROR:
        do_get_errcodes(command);
	break;
      case Q_REPLACE:
	do_get_replace(command);
	break;
      case Q_REPLACE_REGEX:
        do_get_replace_regex(command);
        break;
      case Q_REPLACE_COLUMN:
	do_get_replace_column(command);
	break;
      case Q_SAVE_MASTER_POS: do_save_master_pos(); break;
      case Q_SYNC_WITH_MASTER: do_sync_with_master(command); break;
      case Q_SYNC_SLAVE_WITH_MASTER:
      {
	do_save_master_pos();
	if (*command->first_argument)
	  select_connection(command);
	else
	  select_connection_name("slave");
	do_sync_with_master2(command, 0);
	break;
      }
      case Q_COMMENT:
      {
        command->last_argument= command->end;

        /* Don't output comments in v1 */
        if (opt_result_format_version == 1)
          break;

        /* Don't output comments if query logging is off */
        if (disable_query_log)
          break;

        /* Write comment's with two starting #'s to result file */
        const char* p= command->query;
        if (p && *p == '#' && *(p+1) == '#')
        {
          dynstr_append_mem(&ds_res, command->query, command->query_len);
          dynstr_append(&ds_res, "\n");
        }
	break;
      }
      case Q_EMPTY_LINE:
        /* Don't output newline in v1 */
        if (opt_result_format_version == 1)
          break;

        /* Don't output newline if query logging is off */
        if (disable_query_log)
          break;

        dynstr_append(&ds_res, "\n");
        break;
      case Q_PING:
        handle_command_error(command, mysql_ping(&cur_con->mysql));
        break;
      case Q_RESET_CONNECTION:
        do_reset_connection();
        break;
      case Q_SEND_SHUTDOWN:
        handle_command_error(command,
                             mysql_shutdown(&cur_con->mysql,
                                            SHUTDOWN_DEFAULT));
        break;
      case Q_SHUTDOWN_SERVER:
        do_shutdown_server(command);
        break;
      case Q_EXEC:
      case Q_EXECW:
	do_exec(command);
	command_executed++;
	break;
      case Q_START_TIMER:
	/* Overwrite possible earlier start of timer */
	timer_start= timer_now();
	break;
      case Q_END_TIMER:
	/* End timer before ending mysqltest */
	timer_output();
	break;
      case Q_CHARACTER_SET:
	do_set_charset(command);
	break;
      case Q_DISABLE_PS_PROTOCOL:
        set_property(command, P_PS, 0);
        /* Close any open statements */
        close_statements();
        break;
      case Q_ENABLE_PS_PROTOCOL:
        set_property(command, P_PS, ps_protocol);
        break;
      case Q_DISABLE_RECONNECT:
        set_reconnect(&cur_con->mysql, 0);
        break;
      case Q_ENABLE_RECONNECT:
        set_reconnect(&cur_con->mysql, 1);
        /* Close any open statements - no reconnect, need new prepare */
        close_statements();
        break;
      case Q_DISABLE_PARSING:
        if (parsing_disabled == 0)
          parsing_disabled= 1;
        else
          die("Parsing is already disabled");
        break;
      case Q_ENABLE_PARSING:
        /*
          Ensure we don't get parsing_disabled < 0 as this would accidentally
          disable code we don't want to have disabled
        */
        if (parsing_disabled == 1)
          parsing_disabled= 0;
        else
          die("Parsing is already enabled");
        break;
      case Q_DIE:
        /* Abort test with error code and error message */
        die("%s", command->first_argument);
        break;
      case Q_EXIT:
        /* Stop processing any more commands */
        abort_flag= 1;
        break;
      case Q_SKIP:
        abort_not_supported_test("%s", command->first_argument);
        break;

      case Q_RESULT:
        die("result, deprecated command");
        break;

      case Q_OUTPUT:
        {
          static DYNAMIC_STRING ds_to_file;
          const struct command_arg output_file_args[] = 
            {{ "to_file", ARG_STRING, TRUE, &ds_to_file, "Output filename" }};
          check_command_args(command, command->first_argument,
                             output_file_args, 1, ' ');
          strmake(output_file, ds_to_file.str, FN_REFLEN);
          dynstr_free(&ds_to_file);
          break;
        }

      default:
        processed= 0;
        break;
      }
    }

    if (!processed)
    {
      current_line_inc= 0;
      switch (command->type) {
      case Q_WHILE: do_block(cmd_while, command); break;
      case Q_IF: do_block(cmd_if, command); break;
      case Q_END_BLOCK: do_done(command); break;
      default: current_line_inc = 1; break;
      }
    }
    else
      check_eol_junk(command->last_argument);

    if (command->type != Q_ERROR &&
        command->type != Q_COMMENT)
    {
      /*
        As soon as any non "error" command or comment has been executed,
        the array with expected errors should be cleared
      */
      memset(&saved_expected_errors, 0, sizeof(saved_expected_errors));
    }

    if (command_executed != last_command_executed || command->used_replace)
    {
      /*
        As soon as any command has been executed,
        the replace structures should be cleared
      */
      free_all_replace();

      /* Also reset "sorted_result" and "lowercase"*/
      display_result_sorted= FALSE;
      display_result_lower= FALSE;
    }
    last_command_executed= command_executed;

    parser.current_line += current_line_inc;
    if ( opt_mark_progress )
      mark_progress(command, parser.current_line);

    /* Write result from command to log file immediately */
    log_file.write(&ds_res);
    log_file.flush();
    dynstr_set(&ds_res, 0);
  }

  log_file.close();

  start_lineno= 0;
  verbose_msg("... Done processing test commands.");

  if (parsing_disabled)
    die("Test ended with parsing disabled");

  my_bool empty_result= FALSE;
  
  /*
    The whole test has been executed _sucessfully_.
    Time to compare result or save it to record file.
    The entire output from test is in the log file
  */
  if (log_file.bytes_written())
  {
    if (result_file_name)
    {
      /* A result file has been specified */

      if (record)
      {
	/* Recording */

        /* save a copy of the log to result file */
        if (my_copy(log_file.file_name(), result_file_name, MYF(0)) != 0)
          die("Failed to copy '%s' to '%s', errno: %d",
              log_file.file_name(), result_file_name, errno);

      }
      else
      {
	/* Check that the output from test is equal to result file */
	check_result();
      }
    }
  }
  else
  {
    /* Empty output is an error *unless* we also have an empty result file */
    if (! result_file_name || record ||
        compare_files (log_file.file_name(), result_file_name))
    {
      die("The test didn't produce any output");
    }
    else 
    {
      empty_result= TRUE;  /* Meaning empty was expected */
    }
  }

  if (!command_executed && result_file_name && !empty_result)
    die("No queries executed but non-empty result file found!");

  verbose_msg("Test has succeeded!");
  timer_output();
  /* Yes, if we got this far the test has suceeded! Sakila smiles */
  cleanup_and_exit(0);
  return 0; /* Keep compiler happy too */
}