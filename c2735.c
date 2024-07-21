static MYSQL *db_connect(char *host, char *database,
                         char *user, char *passwd)
{
  MYSQL *mysql;
  if (verbose)
    fprintf(stdout, "Connecting to %s\n", host ? host : "localhost");
  if (!(mysql= mysql_init(NULL)))
    return 0;
  if (opt_compress)
    mysql_options(mysql,MYSQL_OPT_COMPRESS,NullS);
  if (opt_local_file)
    mysql_options(mysql,MYSQL_OPT_LOCAL_INFILE,
		  (char*) &opt_local_file);
#ifdef HAVE_OPENSSL
  if (opt_use_ssl)
    mysql_ssl_set(mysql, opt_ssl_key, opt_ssl_cert, opt_ssl_ca,
		  opt_ssl_capath, opt_ssl_cipher);
  mysql_options(mysql,MYSQL_OPT_SSL_VERIFY_SERVER_CERT,
                (char*)&opt_ssl_verify_server_cert);
#endif
  if (opt_protocol)
    mysql_options(mysql,MYSQL_OPT_PROTOCOL,(char*)&opt_protocol);
#ifdef HAVE_SMEM
  if (shared_memory_base_name)
    mysql_options(mysql,MYSQL_SHARED_MEMORY_BASE_NAME,shared_memory_base_name);
#endif

  if (opt_plugin_dir && *opt_plugin_dir)
    mysql_options(mysql, MYSQL_PLUGIN_DIR, opt_plugin_dir);

  if (opt_default_auth && *opt_default_auth)
    mysql_options(mysql, MYSQL_DEFAULT_AUTH, opt_default_auth);

  if (using_opt_enable_cleartext_plugin)
    mysql_options(mysql, MYSQL_ENABLE_CLEARTEXT_PLUGIN,
                  (char*)&opt_enable_cleartext_plugin);

  mysql_options(mysql, MYSQL_SET_CHARSET_NAME, default_charset);
  if (!(mysql_real_connect(mysql,host,user,passwd,
                           database,opt_mysql_port,opt_mysql_unix_port,
                           0)))
  {
    ignore_errors=0;	  /* NO RETURN FROM db_error */
    db_error(mysql);
  }
  mysql->reconnect= 0;
  if (verbose)
    fprintf(stdout, "Selecting database %s\n", database);
  if (mysql_select_db(mysql, database))
  {
    ignore_errors=0;
    db_error(mysql);
  }
  return mysql;
}