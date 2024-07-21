void safe_connect(MYSQL* mysql, const char *name, const char *host,
                  const char *user, const char *pass, const char *db,
                  int port, const char *sock)
{
  int failed_attempts= 0;

  DBUG_ENTER("safe_connect");

  verbose_msg("Connecting to server %s:%d (socket %s) as '%s'"
              ", connection '%s', attempt %d ...", 
              host, port, sock, user, name, failed_attempts);
  while(!mysql_connect_ssl_check(mysql, host,user, pass, db, port, sock,
                                 CLIENT_MULTI_STATEMENTS | CLIENT_REMEMBER_OPTIONS,
                                 opt_ssl_required))
  {
    /*
      Connect failed

      Only allow retry if this was an error indicating the server
      could not be contacted. Error code differs depending
      on protocol/connection type
    */

    if ((mysql_errno(mysql) == CR_CONN_HOST_ERROR ||
         mysql_errno(mysql) == CR_CONNECTION_ERROR) &&
        failed_attempts < opt_max_connect_retries)
    {
      verbose_msg("Connect attempt %d/%d failed: %d: %s", failed_attempts,
                  opt_max_connect_retries, mysql_errno(mysql),
                  mysql_error(mysql));
      my_sleep(connection_retry_sleep);
    }
    else
    {
      if (failed_attempts > 0)
        die("Could not open connection '%s' after %d attempts: %d %s", name,
            failed_attempts, mysql_errno(mysql), mysql_error(mysql));
      else
        die("Could not open connection '%s': %d %s", name,
            mysql_errno(mysql), mysql_error(mysql));
    }
    failed_attempts++;
  }
  verbose_msg("... Connected.");
  DBUG_VOID_RETURN;
}