static int connect_to_db(char *host, char *user,char *passwd)
{
  char buff[20+FN_REFLEN];
  DBUG_ENTER("connect_to_db");

  verbose_msg("-- Connecting to %s...\n", host ? host : "localhost");
  mysql_init(&mysql_connection);
  if (opt_compress)
    mysql_options(&mysql_connection,MYSQL_OPT_COMPRESS,NullS);
#ifdef HAVE_OPENSSL
  if (opt_use_ssl)
  {
    mysql_ssl_set(&mysql_connection, opt_ssl_key, opt_ssl_cert, opt_ssl_ca,
                  opt_ssl_capath, opt_ssl_cipher);
    mysql_options(&mysql_connection, MYSQL_OPT_SSL_CRL, opt_ssl_crl);
    mysql_options(&mysql_connection, MYSQL_OPT_SSL_CRLPATH, opt_ssl_crlpath);
  }
  mysql_options(&mysql_connection,MYSQL_OPT_SSL_VERIFY_SERVER_CERT,
                (char*)&opt_ssl_verify_server_cert);
#endif
  if (opt_protocol)
    mysql_options(&mysql_connection,MYSQL_OPT_PROTOCOL,(char*)&opt_protocol);
  if (opt_bind_addr)
    mysql_options(&mysql_connection,MYSQL_OPT_BIND,opt_bind_addr);
  if (!opt_secure_auth)
    mysql_options(&mysql_connection,MYSQL_SECURE_AUTH,(char*)&opt_secure_auth);
#ifdef HAVE_SMEM
  if (shared_memory_base_name)
    mysql_options(&mysql_connection,MYSQL_SHARED_MEMORY_BASE_NAME,shared_memory_base_name);
#endif
  mysql_options(&mysql_connection, MYSQL_SET_CHARSET_NAME, default_charset);

  if (opt_plugin_dir && *opt_plugin_dir)
    mysql_options(&mysql_connection, MYSQL_PLUGIN_DIR, opt_plugin_dir);

  if (opt_default_auth && *opt_default_auth)
    mysql_options(&mysql_connection, MYSQL_DEFAULT_AUTH, opt_default_auth);

  if (using_opt_enable_cleartext_plugin)
    mysql_options(&mysql_connection, MYSQL_ENABLE_CLEARTEXT_PLUGIN,
                  (char *) &opt_enable_cleartext_plugin);

  mysql_options(&mysql_connection, MYSQL_OPT_CONNECT_ATTR_RESET, 0);
  mysql_options4(&mysql_connection, MYSQL_OPT_CONNECT_ATTR_ADD,
                 "program_name", "mysqldump");
  if (!(mysql= mysql_connect_ssl_check(&mysql_connection, host, user,
                                       passwd, NULL, opt_mysql_port,
                                       opt_mysql_unix_port, 0,
                                       opt_ssl_required)))
  {
    DB_error(&mysql_connection, "when trying to connect");
    DBUG_RETURN(1);
  }
  if ((mysql_get_server_version(&mysql_connection) < 40100) ||
      (opt_compatible_mode & 3))
  {
    /* Don't dump SET NAMES with a pre-4.1 server (bug#7997).  */
    opt_set_charset= 0;

    /* Don't switch charsets for 4.1 and earlier.  (bug#34192). */
    server_supports_switching_charsets= FALSE;
  } 
  /*
    As we're going to set SQL_MODE, it would be lost on reconnect, so we
    cannot reconnect.
  */
  mysql->reconnect= 0;
  my_snprintf(buff, sizeof(buff), "/*!40100 SET @@SQL_MODE='%s' */",
              compatible_mode_normal_str);
  if (mysql_query_with_error_report(mysql, 0, buff))
    DBUG_RETURN(1);
  /*
    set time_zone to UTC to allow dumping date types between servers with
    different time zone settings
  */
  if (opt_tz_utc)
  {
    my_snprintf(buff, sizeof(buff), "/*!40103 SET TIME_ZONE='+00:00' */");
    if (mysql_query_with_error_report(mysql, 0, buff))
      DBUG_RETURN(1);
  }
  DBUG_RETURN(0);
} /* connect_to_db */