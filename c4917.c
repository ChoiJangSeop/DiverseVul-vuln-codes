static void
mysql_ssl_free(MYSQL *mysql MY_ATTRIBUTE((unused)))
{
  struct st_VioSSLFd *ssl_fd= (struct st_VioSSLFd*) mysql->connector_fd;
  DBUG_ENTER("mysql_ssl_free");

  my_free(mysql->options.ssl_key);
  my_free(mysql->options.ssl_cert);
  my_free(mysql->options.ssl_ca);
  my_free(mysql->options.ssl_capath);
  my_free(mysql->options.ssl_cipher);
  if (mysql->options.extension)
  {
    my_free(mysql->options.extension->ssl_crl);
    my_free(mysql->options.extension->ssl_crlpath);
  }
  if (ssl_fd)
    SSL_CTX_free(ssl_fd->ssl_context);
  my_free(mysql->connector_fd);
  mysql->options.ssl_key = 0;
  mysql->options.ssl_cert = 0;
  mysql->options.ssl_ca = 0;
  mysql->options.ssl_capath = 0;
  mysql->options.ssl_cipher= 0;
  if (mysql->options.extension)
  {
    mysql->options.extension->ssl_crl = 0;
    mysql->options.extension->ssl_crlpath = 0;
  }
  mysql->options.use_ssl = FALSE;
  mysql->connector_fd = 0;
  DBUG_VOID_RETURN;