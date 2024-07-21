sql_real_connect(char *host,char *database,char *user,char *password,
		 uint silent)
{
  if (connected)
  {
    connected= 0;
    mysql_close(&mysql);
  }
  mysql_init(&mysql);
  if (opt_init_command)
    mysql_options(&mysql, MYSQL_INIT_COMMAND, opt_init_command);
  if (opt_connect_timeout)
  {
    uint timeout=opt_connect_timeout;
    mysql_options(&mysql,MYSQL_OPT_CONNECT_TIMEOUT,
		  (char*) &timeout);
  }
  if (opt_compress)
    mysql_options(&mysql,MYSQL_OPT_COMPRESS,NullS);
  if (opt_secure_auth)
    mysql_options(&mysql, MYSQL_SECURE_AUTH, (char *) &opt_secure_auth);
  if (using_opt_local_infile)
    mysql_options(&mysql,MYSQL_OPT_LOCAL_INFILE, (char*) &opt_local_infile);
#if defined(HAVE_OPENSSL) && !defined(EMBEDDED_LIBRARY)
  if (opt_use_ssl)
    mysql_ssl_set(&mysql, opt_ssl_key, opt_ssl_cert, opt_ssl_ca,
		  opt_ssl_capath, opt_ssl_cipher);
  mysql_options(&mysql,MYSQL_OPT_SSL_VERIFY_SERVER_CERT,
                (char*)&opt_ssl_verify_server_cert);
#endif
  if (opt_protocol)
    mysql_options(&mysql,MYSQL_OPT_PROTOCOL,(char*)&opt_protocol);
#ifdef HAVE_SMEM
  if (shared_memory_base_name)
    mysql_options(&mysql,MYSQL_SHARED_MEMORY_BASE_NAME,shared_memory_base_name);
#endif
  if (safe_updates)
  {
    char init_command[100];
    sprintf(init_command,
	    "SET SQL_SAFE_UPDATES=1,SQL_SELECT_LIMIT=%lu,MAX_JOIN_SIZE=%lu",
	    select_limit,max_join_size);
    mysql_options(&mysql, MYSQL_INIT_COMMAND, init_command);
  }

  mysql_options(&mysql, MYSQL_SET_CHARSET_NAME, default_charset);
  
  if (opt_plugin_dir && *opt_plugin_dir)
    mysql_options(&mysql, MYSQL_PLUGIN_DIR, opt_plugin_dir);

  if (opt_default_auth && *opt_default_auth)
    mysql_options(&mysql, MYSQL_DEFAULT_AUTH, opt_default_auth);

  if (using_opt_enable_cleartext_plugin)
    mysql_options(&mysql, MYSQL_ENABLE_CLEARTEXT_PLUGIN, 
                  (char*) &opt_enable_cleartext_plugin);

  if (!mysql_connect_ssl_check(&mysql, host, user, password,
                               database, opt_mysql_port, opt_mysql_unix_port,
                               connect_flag | CLIENT_MULTI_STATEMENTS,
                               opt_ssl_required))
  {
    if (!silent ||
	(mysql_errno(&mysql) != CR_CONN_HOST_ERROR &&
	 mysql_errno(&mysql) != CR_CONNECTION_ERROR))
    {
      (void) put_error(&mysql);
      (void) fflush(stdout);
      return ignore_errors ? -1 : 1;		// Abort
    }
    return -1;					// Retryable
  }
  
  charset_info= mysql.charset;
  
  connected=1;
#ifndef EMBEDDED_LIBRARY
  mysql.reconnect= debug_info_flag; // We want to know if this happens
#else
  mysql.reconnect= 1;
#endif
#ifdef HAVE_READLINE
  build_completion_hash(opt_rehash, 1);
#endif
  return 0;
}