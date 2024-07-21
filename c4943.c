MYSQL *mysql_connect_ssl_check(MYSQL *mysql_arg, const char *host,
                               const char *user, const char *passwd,
                               const char *db, uint port,
                               const char *unix_socket, ulong client_flag,
                               my_bool ssl_required __attribute__((unused)))
{
  MYSQL *mysql= mysql_real_connect(mysql_arg, host, user, passwd, db, port,
                                   unix_socket, client_flag);
#if defined(HAVE_OPENSSL) && !defined(EMBEDDED_LIBRARY)
  if (mysql &&                                   /* connection established. */
      ssl_required &&                            /* --ssl-mode=REQUIRED. */
      !mysql_get_ssl_cipher(mysql))              /* non-SSL connection. */
  {
    NET *net= &mysql->net;
    net->last_errno= CR_SSL_CONNECTION_ERROR;
    strmov(net->last_error, "--ssl-mode=REQUIRED option forbids non SSL connections");
    strmov(net->sqlstate, "HY000");
    return NULL;
  }
#endif
  return mysql;
}