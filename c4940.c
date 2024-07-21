
void mysql_read_default_options(struct st_mysql_options *options,
				const char *filename,const char *group)
{
  int argc;
  char *argv_buff[1],**argv;
  const char *groups[3];
  DBUG_ENTER("mysql_read_default_options");
  DBUG_PRINT("enter",("file: %s  group: %s",filename,group ? group :"NULL"));

  compile_time_assert(OPT_keep_this_one_last ==
                      array_elements(default_options));

  argc=1; argv=argv_buff; argv_buff[0]= (char*) "client";
  groups[0]= (char*) "client"; groups[1]= (char*) group; groups[2]=0;

  my_load_defaults(filename, groups, &argc, &argv, NULL);
  if (argc != 1)				/* If some default option */
  {
    char **option=argv;
    while (*++option)
    {
      if (my_getopt_is_args_separator(option[0]))          /* skip arguments separator */
        continue;
      /* DBUG_PRINT("info",("option: %s",option[0])); */
      if (option[0][0] == '-' && option[0][1] == '-')
      {
	char *end=strcend(*option,'=');
	char *opt_arg=0;
	if (*end)
	{
	  opt_arg=end+1;
	  *end=0;				/* Remove '=' */
	}
	/* Change all '_' in variable name to '-' */
	for (end= *option ; *(end= strcend(end,'_')) ; )
	  *end= '-';
	switch (find_type(*option + 2, &option_types, FIND_TYPE_BASIC)) {
	case OPT_port:
	  if (opt_arg)
	    options->port=atoi(opt_arg);
	  break;
	case OPT_socket:
	  if (opt_arg)
	  {
	    my_free(options->unix_socket);
	    options->unix_socket=my_strdup(opt_arg,MYF(MY_WME));
	  }
	  break;
	case OPT_compress:
	  options->compress=1;
	  options->client_flag|= CLIENT_COMPRESS;
	  break;
        case OPT_password:
	  if (opt_arg)
	  {
	    my_free(options->password);
	    options->password=my_strdup(opt_arg,MYF(MY_WME));
	  }
	  break;
        case OPT_pipe:
          options->protocol = MYSQL_PROTOCOL_PIPE;
	case OPT_connect_timeout:
	case OPT_timeout:
	  if (opt_arg)
	    options->connect_timeout=atoi(opt_arg);
	  break;
	case OPT_user:
	  if (opt_arg)
	  {
	    my_free(options->user);
	    options->user=my_strdup(opt_arg,MYF(MY_WME));
	  }
	  break;
	case OPT_init_command:
	  add_init_command(options,opt_arg);
	  break;
	case OPT_host:
	  if (opt_arg)
	  {
	    my_free(options->host);
	    options->host=my_strdup(opt_arg,MYF(MY_WME));
	  }
	  break;
	case OPT_database:
	  if (opt_arg)
	  {
	    my_free(options->db);
	    options->db=my_strdup(opt_arg,MYF(MY_WME));
	  }
	  break;
	case OPT_debug:
#ifdef MYSQL_CLIENT
	  mysql_debug(opt_arg ? opt_arg : "d:t:o,/tmp/client.trace");
	  break;
#endif
	case OPT_return_found_rows:
	  options->client_flag|=CLIENT_FOUND_ROWS;
	  break;
#if defined(HAVE_OPENSSL) && !defined(EMBEDDED_LIBRARY)
	case OPT_ssl_key:
	  my_free(options->ssl_key);
          options->ssl_key = my_strdup(opt_arg, MYF(MY_WME));
          break;
	case OPT_ssl_cert:
	  my_free(options->ssl_cert);
          options->ssl_cert = my_strdup(opt_arg, MYF(MY_WME));
          break;
	case OPT_ssl_ca:
	  my_free(options->ssl_ca);
          options->ssl_ca = my_strdup(opt_arg, MYF(MY_WME));
          break;
	case OPT_ssl_capath:
	  my_free(options->ssl_capath);
          options->ssl_capath = my_strdup(opt_arg, MYF(MY_WME));
          break;
        case OPT_ssl_cipher:
          my_free(options->ssl_cipher);
          options->ssl_cipher= my_strdup(opt_arg, MYF(MY_WME));
          break;
#else
	case OPT_ssl_key:
	case OPT_ssl_cert:
	case OPT_ssl_ca:
	case OPT_ssl_capath:
        case OPT_ssl_cipher:
	  break;
#endif /* HAVE_OPENSSL && !EMBEDDED_LIBRARY */
	case OPT_character_sets_dir:
	  my_free(options->charset_dir);
          options->charset_dir = my_strdup(opt_arg, MYF(MY_WME));
	  break;
	case OPT_default_character_set:
	  my_free(options->charset_name);
          options->charset_name = my_strdup(opt_arg, MYF(MY_WME));
	  break;
	case OPT_interactive_timeout:
	  options->client_flag|= CLIENT_INTERACTIVE;
	  break;
	case OPT_local_infile:
	  if (!opt_arg || atoi(opt_arg) != 0)
	    options->client_flag|= CLIENT_LOCAL_FILES;
	  else
	    options->client_flag&= ~CLIENT_LOCAL_FILES;
	  break;
	case OPT_disable_local_infile:
	  options->client_flag&= ~CLIENT_LOCAL_FILES;
          break;
	case OPT_max_allowed_packet:
          if (opt_arg)
	    options->max_allowed_packet= atoi(opt_arg);
	  break;
        case OPT_protocol:
          if ((options->protocol= find_type(opt_arg, &sql_protocol_typelib,
                                            FIND_TYPE_BASIC)) <= 0)
          {
            fprintf(stderr, "Unknown option to protocol: %s\n", opt_arg);
            exit(1);
          }
          break;
        case OPT_shared_memory_base_name:
#ifdef HAVE_SMEM
          if (options->shared_memory_base_name != def_shared_memory_base_name)
            my_free(options->shared_memory_base_name);
          options->shared_memory_base_name=my_strdup(opt_arg,MYF(MY_WME));
#endif
          break;
	case OPT_multi_results:
	  options->client_flag|= CLIENT_MULTI_RESULTS;
	  break;
	case OPT_multi_statements:
	case OPT_multi_queries:
	  options->client_flag|= CLIENT_MULTI_STATEMENTS | CLIENT_MULTI_RESULTS;
	  break;
        case OPT_secure_auth:
          options->secure_auth= TRUE;
          break;
        case OPT_report_data_truncation:
          options->report_data_truncation= opt_arg ? test(atoi(opt_arg)) : 1;
          break;
        case OPT_plugin_dir:
          {
            char buff[FN_REFLEN], buff2[FN_REFLEN];
            if (strlen(opt_arg) >= FN_REFLEN)
              opt_arg[FN_REFLEN]= '\0';
            if (my_realpath(buff, opt_arg, 0))
            {
              DBUG_PRINT("warning",("failed to normalize the plugin path: %s",
                                    opt_arg));
              break;
            }
            convert_dirname(buff2, buff, NULL);
            EXTENSION_SET_STRING(options, plugin_dir, buff2);
          }
          break;
        case OPT_default_auth:
          EXTENSION_SET_STRING(options, default_auth, opt_arg);
          break;

        case OPT_enable_cleartext_plugin:
          ENSURE_EXTENSIONS_PRESENT(options);
          options->extension->enable_cleartext_plugin= 
            (!opt_arg || atoi(opt_arg) != 0) ? TRUE : FALSE;
          break;

	default:
	  DBUG_PRINT("warning",("unknown option: %s",option[0]));
	}
      }
    }
  }
  free_defaults(argv);
  DBUG_VOID_RETURN;