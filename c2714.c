void do_connect(struct st_command *command)
{
  int con_port= opt_port;
  char *con_options;
  my_bool con_ssl= 0, con_compress= 0;
  my_bool con_pipe= 0, con_shm= 0, con_cleartext_enable= 0;
  my_bool con_secure_auth= 1;
  struct st_connection* con_slot;

  static DYNAMIC_STRING ds_connection_name;
  static DYNAMIC_STRING ds_host;
  static DYNAMIC_STRING ds_user;
  static DYNAMIC_STRING ds_password;
  static DYNAMIC_STRING ds_database;
  static DYNAMIC_STRING ds_port;
  static DYNAMIC_STRING ds_sock;
  static DYNAMIC_STRING ds_options;
  static DYNAMIC_STRING ds_default_auth;
#if defined (_WIN32) && !defined (EMBEDDED_LIBRARY)
  static DYNAMIC_STRING ds_shm;
#endif
  const struct command_arg connect_args[] = {
    { "connection name", ARG_STRING, TRUE, &ds_connection_name, "Name of the connection" },
    { "host", ARG_STRING, TRUE, &ds_host, "Host to connect to" },
    { "user", ARG_STRING, FALSE, &ds_user, "User to connect as" },
    { "passsword", ARG_STRING, FALSE, &ds_password, "Password used when connecting" },
    { "database", ARG_STRING, FALSE, &ds_database, "Database to select after connect" },
    { "port", ARG_STRING, FALSE, &ds_port, "Port to connect to" },
    { "socket", ARG_STRING, FALSE, &ds_sock, "Socket to connect with" },
    { "options", ARG_STRING, FALSE, &ds_options, "Options to use while connecting" },
    { "default_auth", ARG_STRING, FALSE, &ds_default_auth, "Default authentication to use" }
  };

  DBUG_ENTER("do_connect");
  DBUG_PRINT("enter",("connect: %s", command->first_argument));

  strip_parentheses(command);
  check_command_args(command, command->first_argument, connect_args,
                     sizeof(connect_args)/sizeof(struct command_arg),
                     ',');

  /* Port */
  if (ds_port.length)
  {
    con_port= atoi(ds_port.str);
    if (con_port == 0)
      die("Illegal argument for port: '%s'", ds_port.str);
  }

#if defined (_WIN32) && !defined (EMBEDDED_LIBRARY)
  /* Shared memory */
  init_dynamic_string(&ds_shm, ds_sock.str, 0, 0);
#endif

  /* Sock */
  if (ds_sock.length)
  {
    /*
      If the socket is specified just as a name without path
      append tmpdir in front
    */
    if (*ds_sock.str != FN_LIBCHAR)
    {
      char buff[FN_REFLEN];
      fn_format(buff, ds_sock.str, TMPDIR, "", 0);
      dynstr_set(&ds_sock, buff);
    }
  }
  else
  {
    /* No socket specified, use default */
    dynstr_set(&ds_sock, unix_sock);
  }
  DBUG_PRINT("info", ("socket: %s", ds_sock.str));


  /* Options */
  con_options= ds_options.str;
  while (*con_options)
  {
    char* end;
    /* Step past any spaces in beginning of option*/
    while (*con_options && my_isspace(charset_info, *con_options))
     con_options++;
    /* Find end of this option */
    end= con_options;
    while (*end && !my_isspace(charset_info, *end))
      end++;
    if (!strncmp(con_options, "SSL", 3))
      con_ssl= 1;
    else if (!strncmp(con_options, "COMPRESS", 8))
      con_compress= 1;
    else if (!strncmp(con_options, "PIPE", 4))
      con_pipe= 1;
    else if (!strncmp(con_options, "SHM", 3))
      con_shm= 1;
    else if (!strncmp(con_options, "CLEARTEXT", 9))
      con_cleartext_enable= 1;
    else if (!strncmp(con_options, "SKIPSECUREAUTH",14))
      con_secure_auth= 0;
    else
      die("Illegal option to connect: %.*s", 
          (int) (end - con_options), con_options);
    /* Process next option */
    con_options= end;
  }

  if (find_connection_by_name(ds_connection_name.str))
    die("Connection %s already exists", ds_connection_name.str);
    
  if (next_con != connections_end)
    con_slot= next_con;
  else
  {
    if (!(con_slot= find_connection_by_name("-closed_connection-")))
      die("Connection limit exhausted, you can have max %d connections",
          opt_max_connections);
  }

#ifdef EMBEDDED_LIBRARY
  init_connection_thd(con_slot);
#endif /*EMBEDDED_LIBRARY*/

  if (!mysql_init(&con_slot->mysql))
    die("Failed on mysql_init()");

  if (opt_connect_timeout)
    mysql_options(&con_slot->mysql, MYSQL_OPT_CONNECT_TIMEOUT,
                  (void *) &opt_connect_timeout);

  if (opt_compress || con_compress)
    mysql_options(&con_slot->mysql, MYSQL_OPT_COMPRESS, NullS);
  mysql_options(&con_slot->mysql, MYSQL_OPT_LOCAL_INFILE, 0);
  mysql_options(&con_slot->mysql, MYSQL_SET_CHARSET_NAME,
                charset_info->csname);
  if (opt_charsets_dir)
    mysql_options(&con_slot->mysql, MYSQL_SET_CHARSET_DIR,
                  opt_charsets_dir);

#if defined(HAVE_OPENSSL) && !defined(EMBEDDED_LIBRARY)
  if (opt_use_ssl)
    con_ssl= 1;
#endif

  if (con_ssl)
  {
#if defined(HAVE_OPENSSL) && !defined(EMBEDDED_LIBRARY)
    mysql_ssl_set(&con_slot->mysql, opt_ssl_key, opt_ssl_cert, opt_ssl_ca,
		  opt_ssl_capath, opt_ssl_cipher);
    mysql_options(&con_slot->mysql, MYSQL_OPT_SSL_CRL, opt_ssl_crl);
    mysql_options(&con_slot->mysql, MYSQL_OPT_SSL_CRLPATH, opt_ssl_crlpath);
#if MYSQL_VERSION_ID >= 50000
    /* Turn on ssl_verify_server_cert only if host is "localhost" */
    opt_ssl_verify_server_cert= !strcmp(ds_host.str, "localhost");
    mysql_options(&con_slot->mysql, MYSQL_OPT_SSL_VERIFY_SERVER_CERT,
                  &opt_ssl_verify_server_cert);
#endif
#endif
  }

  if (con_pipe)
  {
#if defined(_WIN32) && !defined(EMBEDDED_LIBRARY)
    opt_protocol= MYSQL_PROTOCOL_PIPE;
#endif
  }

#ifndef EMBEDDED_LIBRARY
  if (opt_protocol)
    mysql_options(&con_slot->mysql, MYSQL_OPT_PROTOCOL, (char*) &opt_protocol);
#endif

  if (con_shm)
  {
#if defined (_WIN32) && !defined (EMBEDDED_LIBRARY)
    uint protocol= MYSQL_PROTOCOL_MEMORY;
    if (!ds_shm.length)
      die("Missing shared memory base name");
    mysql_options(&con_slot->mysql, MYSQL_SHARED_MEMORY_BASE_NAME, ds_shm.str);
    mysql_options(&con_slot->mysql, MYSQL_OPT_PROTOCOL, &protocol);
#endif
  }
#if defined (_WIN32) && !defined (EMBEDDED_LIBRARY)
  else if (shared_memory_base_name)
  {
    mysql_options(&con_slot->mysql, MYSQL_SHARED_MEMORY_BASE_NAME,
                  shared_memory_base_name);
  }
#endif

  /* Use default db name */
  if (ds_database.length == 0)
    dynstr_set(&ds_database, opt_db);

  if (opt_plugin_dir && *opt_plugin_dir)
    mysql_options(&con_slot->mysql, MYSQL_PLUGIN_DIR, opt_plugin_dir);

  if (ds_default_auth.length)
    mysql_options(&con_slot->mysql, MYSQL_DEFAULT_AUTH, ds_default_auth.str);

#if !defined(HAVE_YASSL)
  /* Set server public_key */
  if (opt_server_public_key && *opt_server_public_key)
    mysql_options(&con_slot->mysql, MYSQL_SERVER_PUBLIC_KEY,
                  opt_server_public_key);
#endif
  
  if (con_cleartext_enable)
    mysql_options(&con_slot->mysql, MYSQL_ENABLE_CLEARTEXT_PLUGIN,
                  (char*) &con_cleartext_enable);

  if (!con_secure_auth)
    mysql_options(&con_slot->mysql, MYSQL_SECURE_AUTH,
                  (char*) &con_secure_auth);

  /* Special database to allow one to connect without a database name */
  if (ds_database.length && !strcmp(ds_database.str,"*NO-ONE*"))
    dynstr_set(&ds_database, "");

  if (connect_n_handle_errors(command, &con_slot->mysql,
                              ds_host.str,ds_user.str,
                              ds_password.str, ds_database.str,
                              con_port, ds_sock.str))
  {
    DBUG_PRINT("info", ("Inserting connection %s in connection pool",
                        ds_connection_name.str));
    if (!(con_slot->name= my_strdup(PSI_NOT_INSTRUMENTED,
                                    ds_connection_name.str, MYF(MY_WME))))
      die("Out of memory");
    con_slot->name_len= strlen(con_slot->name);
    set_current_connection(con_slot);

    if (con_slot == next_con)
      next_con++; /* if we used the next_con slot, advance the pointer */
  }

  dynstr_free(&ds_connection_name);
  dynstr_free(&ds_host);
  dynstr_free(&ds_user);
  dynstr_free(&ds_password);
  dynstr_free(&ds_database);
  dynstr_free(&ds_port);
  dynstr_free(&ds_sock);
  dynstr_free(&ds_options);
  dynstr_free(&ds_default_auth);
#if defined (_WIN32) && !defined (EMBEDDED_LIBRARY)
  dynstr_free(&ds_shm);
#endif
  DBUG_VOID_RETURN;
}