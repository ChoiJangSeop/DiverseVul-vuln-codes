static int dbConnect(char *host, char *user, char *passwd)
{
  DBUG_ENTER("dbConnect");
  if (verbose)
  {
    fprintf(stderr, "# Connecting to %s...\n", host ? host : "localhost");
  }
  mysql_init(&mysql_connection);
  if (opt_compress)
    mysql_options(&mysql_connection, MYSQL_OPT_COMPRESS, NullS);
#ifdef HAVE_OPENSSL
  if (opt_use_ssl)
  {
    mysql_ssl_set(&mysql_connection, opt_ssl_key, opt_ssl_cert, opt_ssl_ca,
		  opt_ssl_capath, opt_ssl_cipher);
    mysql_options(&mysql_connection, MYSQL_OPT_SSL_CRL, opt_ssl_crl);
    mysql_options(&mysql_connection, MYSQL_OPT_SSL_CRLPATH, opt_ssl_crlpath);
  }
#endif
  if (opt_protocol)
    mysql_options(&mysql_connection,MYSQL_OPT_PROTOCOL,(char*)&opt_protocol);
  if (opt_bind_addr)
    mysql_options(&mysql_connection, MYSQL_OPT_BIND, opt_bind_addr);
  if (!opt_secure_auth)
    mysql_options(&mysql_connection, MYSQL_SECURE_AUTH,(char*)&opt_secure_auth);
#ifdef HAVE_SMEM
  if (shared_memory_base_name)
    mysql_options(&mysql_connection,MYSQL_SHARED_MEMORY_BASE_NAME,shared_memory_base_name);
#endif

  if (opt_plugin_dir && *opt_plugin_dir)
    mysql_options(&mysql_connection, MYSQL_PLUGIN_DIR, opt_plugin_dir);

  if (opt_default_auth && *opt_default_auth)
    mysql_options(&mysql_connection, MYSQL_DEFAULT_AUTH, opt_default_auth);

  if (using_opt_enable_cleartext_plugin)
    mysql_options(&mysql_connection, MYSQL_ENABLE_CLEARTEXT_PLUGIN,
                  (char *) &opt_enable_cleartext_plugin);

  mysql_options(&mysql_connection, MYSQL_SET_CHARSET_NAME, default_charset);
  mysql_options(&mysql_connection, MYSQL_OPT_CONNECT_ATTR_RESET, 0);
  mysql_options4(&mysql_connection, MYSQL_OPT_CONNECT_ATTR_ADD,
                 "program_name", "mysqlcheck");
  if (!(sock = mysql_connect_ssl_check(&mysql_connection, host, user, passwd,
                                       NULL, opt_mysql_port,
                                       opt_mysql_unix_port, 0,
                                       opt_ssl_required)))
  {
    DBerror(&mysql_connection, "when trying to connect");
    DBUG_RETURN(1);
  }
  mysql_connection.reconnect= 1;
  DBUG_RETURN(0);
} /* dbConnect */