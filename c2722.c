static int connect_to_master(THD* thd, MYSQL* mysql, Master_info* mi,
                             bool reconnect, bool suppress_warnings)
{
  int slave_was_killed= 0;
  int last_errno= -2;                           // impossible error
  ulong err_count=0;
  char llbuff[22];
  char password[MAX_PASSWORD_LENGTH + 1];
  int password_size= sizeof(password);
  DBUG_ENTER("connect_to_master");
  set_slave_max_allowed_packet(thd, mysql);
#ifndef DBUG_OFF
  mi->events_until_exit = disconnect_slave_event_count;
#endif
  ulong client_flag= CLIENT_REMEMBER_OPTIONS;
  if (opt_slave_compressed_protocol)
    client_flag=CLIENT_COMPRESS;                /* We will use compression */

  mysql_options(mysql, MYSQL_OPT_CONNECT_TIMEOUT, (char *) &slave_net_timeout);
  mysql_options(mysql, MYSQL_OPT_READ_TIMEOUT, (char *) &slave_net_timeout);

  if (mi->bind_addr[0])
  {
    DBUG_PRINT("info",("bind_addr: %s", mi->bind_addr));
    mysql_options(mysql, MYSQL_OPT_BIND, mi->bind_addr);
  }

#ifdef HAVE_OPENSSL
  if (mi->ssl)
  {
    mysql_ssl_set(mysql,
                  mi->ssl_key[0]?mi->ssl_key:0,
                  mi->ssl_cert[0]?mi->ssl_cert:0,
                  mi->ssl_ca[0]?mi->ssl_ca:0,
                  mi->ssl_capath[0]?mi->ssl_capath:0,
                  mi->ssl_cipher[0]?mi->ssl_cipher:0);
    mysql_options(mysql, MYSQL_OPT_SSL_CRL, 
                  mi->ssl_crl[0] ? mi->ssl_crl : 0);
    mysql_options(mysql, MYSQL_OPT_SSL_CRLPATH, 
                  mi->ssl_crlpath[0] ? mi->ssl_crlpath : 0);
    mysql_options(mysql, MYSQL_OPT_SSL_VERIFY_SERVER_CERT,
                  &mi->ssl_verify_server_cert);
  }
#endif

  mysql_options(mysql, MYSQL_SET_CHARSET_NAME, default_charset_info->csname);
  /* This one is not strictly needed but we have it here for completeness */
  mysql_options(mysql, MYSQL_SET_CHARSET_DIR, (char *) charsets_dir);

  if (mi->is_start_plugin_auth_configured())
  {
    DBUG_PRINT("info", ("Slaving is using MYSQL_DEFAULT_AUTH %s",
                        mi->get_start_plugin_auth()));
    mysql_options(mysql, MYSQL_DEFAULT_AUTH, mi->get_start_plugin_auth());
  }
  
  if (mi->is_start_plugin_dir_configured())
  {
    DBUG_PRINT("info", ("Slaving is using MYSQL_PLUGIN_DIR %s",
                        mi->get_start_plugin_dir()));
    mysql_options(mysql, MYSQL_PLUGIN_DIR, mi->get_start_plugin_dir());
  }
  /* Set MYSQL_PLUGIN_DIR in case master asks for an external authentication plugin */
  else if (opt_plugin_dir_ptr && *opt_plugin_dir_ptr)
    mysql_options(mysql, MYSQL_PLUGIN_DIR, opt_plugin_dir_ptr);
  
  if (!mi->is_start_user_configured())
    sql_print_warning("%s", ER(ER_INSECURE_CHANGE_MASTER));

  if (mi->get_password(password, &password_size))
  {
    mi->report(ERROR_LEVEL, ER_SLAVE_FATAL_ERROR,
               ER(ER_SLAVE_FATAL_ERROR),
               "Unable to configure password when attempting to "
               "connect to the master server. Connection attempt "
               "terminated.");
    DBUG_RETURN(1);
  }

  const char* user= mi->get_user();
  if (user == NULL || user[0] == 0)
  {
    mi->report(ERROR_LEVEL, ER_SLAVE_FATAL_ERROR,
               ER(ER_SLAVE_FATAL_ERROR),
               "Invalid (empty) username when attempting to "
               "connect to the master server. Connection attempt "
               "terminated.");
    DBUG_RETURN(1);
  }

  while (!(slave_was_killed = io_slave_killed(thd,mi))
         && (reconnect ? mysql_reconnect(mysql) != 0 :
             mysql_real_connect(mysql, mi->host, user,
                                password, 0, mi->port, 0, client_flag) == 0))
  {
    /*
       SHOW SLAVE STATUS will display the number of retries which
       would be real retry counts instead of mi->retry_count for
       each connection attempt by 'Last_IO_Error' entry.
    */
    last_errno=mysql_errno(mysql);
    suppress_warnings= 0;
    mi->report(ERROR_LEVEL, last_errno,
               "error %s to master '%s@%s:%d'"
               " - retry-time: %d  retries: %lu",
               (reconnect ? "reconnecting" : "connecting"),
               mi->get_user(), mi->host, mi->port,
               mi->connect_retry, err_count + 1);
    /*
      By default we try forever. The reason is that failure will trigger
      master election, so if the user did not set mi->retry_count we
      do not want to have election triggered on the first failure to
      connect
    */
    if (++err_count == mi->retry_count)
    {
      slave_was_killed=1;
      break;
    }
    slave_sleep(thd, mi->connect_retry, io_slave_killed, mi);
  }

  if (!slave_was_killed)
  {
    mi->clear_error(); // clear possible left over reconnect error
    if (reconnect)
    {
      if (!suppress_warnings)
        sql_print_information("Slave: connected to master '%s@%s:%d',\
replication resumed in log '%s' at position %s", mi->get_user(),
                              mi->host, mi->port,
                              mi->get_io_rpl_log_name(),
                              llstr(mi->get_master_log_pos(),llbuff));
    }
    else
    {
      query_logger.general_log_print(thd, COM_CONNECT_OUT, "%s@%s:%d",
                                     mi->get_user(), mi->host, mi->port);
    }

    thd->set_active_vio(mysql->net.vio);
  }
  mysql->reconnect= 1;
  DBUG_PRINT("exit",("slave_was_killed: %d", slave_was_killed));
  DBUG_RETURN(slave_was_killed);
}