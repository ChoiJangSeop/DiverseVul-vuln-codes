static int send_client_reply_packet(MCPVIO_EXT *mpvio,
                                    const uchar *data, int data_len)
{
  MYSQL *mysql= mpvio->mysql;
  NET *net= &mysql->net;
  char *buff, *end;

  /* see end= buff+32 below, fixed size of the packet is 32 bytes */
  buff= my_alloca(33 + USERNAME_LENGTH + data_len + NAME_LEN + NAME_LEN);
  
  mysql->client_flag|= mysql->options.client_flag;
  mysql->client_flag|= CLIENT_CAPABILITIES;

  if (mysql->client_flag & CLIENT_MULTI_STATEMENTS)
    mysql->client_flag|= CLIENT_MULTI_RESULTS;

#if defined(HAVE_OPENSSL) && !defined(EMBEDDED_LIBRARY)
  if (mysql->options.use_ssl)
    mysql->client_flag|= CLIENT_SSL;
#endif /* HAVE_OPENSSL && !EMBEDDED_LIBRARY*/

  if (mpvio->db)
    mysql->client_flag|= CLIENT_CONNECT_WITH_DB;

  /* Remove options that server doesn't support */
  mysql->client_flag= mysql->client_flag &
                       (~(CLIENT_COMPRESS | CLIENT_SSL | CLIENT_PROTOCOL_41) 
                       | mysql->server_capabilities);

#ifndef HAVE_COMPRESS
  mysql->client_flag&= ~CLIENT_COMPRESS;
#endif

  if (mysql->client_flag & CLIENT_PROTOCOL_41)
  {
    /* 4.1 server and 4.1 client has a 32 byte option flag */
    int4store(buff,mysql->client_flag);
    int4store(buff+4, net->max_packet_size);
    buff[8]= (char) mysql->charset->number;
    bzero(buff+9, 32-9);
    end= buff+32;
  }
  else
  {
    int2store(buff, mysql->client_flag);
    int3store(buff+2, net->max_packet_size);
    end= buff+5;
  }

  /*
     If client uses ssl and client also has to verify the server
     certificate, a ssl connection is required.
     If the server does not support ssl, we abort the connection.
  */
  if (mysql->options.use_ssl &&
      (mysql->client_flag & CLIENT_SSL_VERIFY_SERVER_CERT) &&
      !(mysql->server_capabilities & CLIENT_SSL))
  {
    set_mysql_extended_error(mysql, CR_SSL_CONNECTION_ERROR, unknown_sqlstate,
                             ER(CR_SSL_CONNECTION_ERROR),
                             "SSL is required, but the server does not "
                             "support it");
    goto error;
  }

#ifdef HAVE_OPENSSL
  if (mysql->client_flag & CLIENT_SSL)
  {
    /* Do the SSL layering. */
    struct st_mysql_options *options= &mysql->options;
    struct st_VioSSLFd *ssl_fd;
    enum enum_ssl_init_error ssl_init_error;
    const char *cert_error;
    unsigned long ssl_error;

    /*
      Send mysql->client_flag, max_packet_size - unencrypted otherwise
      the server does not know we want to do SSL
    */
    if (my_net_write(net, (uchar*)buff, (size_t) (end-buff)) || net_flush(net))
    {
      set_mysql_extended_error(mysql, CR_SERVER_LOST, unknown_sqlstate,
                               ER(CR_SERVER_LOST_EXTENDED),
                               "sending connection information to server",
                               errno);
      goto error;
    }

    /* Create the VioSSLConnectorFd - init SSL and load certs */
    if (!(ssl_fd= new_VioSSLConnectorFd(options->ssl_key,
                                        options->ssl_cert,
                                        options->ssl_ca,
                                        options->ssl_capath,
                                        options->ssl_cipher,
                                        &ssl_init_error)))
    {
      set_mysql_extended_error(mysql, CR_SSL_CONNECTION_ERROR, unknown_sqlstate,
                               ER(CR_SSL_CONNECTION_ERROR), sslGetErrString(ssl_init_error));
      goto error;
    }
    mysql->connector_fd= (unsigned char *) ssl_fd;

    /* Connect to the server */
    DBUG_PRINT("info", ("IO layer change in progress..."));
    if (sslconnect(ssl_fd, net->vio,
                   (long) (mysql->options.connect_timeout), &ssl_error))
    {    
      char buf[512];
      ERR_error_string_n(ssl_error, buf, 512);
      buf[511]= 0;
      set_mysql_extended_error(mysql, CR_SSL_CONNECTION_ERROR, unknown_sqlstate,
                               ER(CR_SSL_CONNECTION_ERROR),
                               buf);
      goto error;
    }
    DBUG_PRINT("info", ("IO layer change done!"));

    /* Verify server cert */
    if ((mysql->client_flag & CLIENT_SSL_VERIFY_SERVER_CERT) &&
        ssl_verify_server_cert(net->vio, mysql->host, &cert_error))
    {
      set_mysql_extended_error(mysql, CR_SSL_CONNECTION_ERROR, unknown_sqlstate,
                               ER(CR_SSL_CONNECTION_ERROR), cert_error);
      goto error;
    }
  }
#endif /* HAVE_OPENSSL */

  DBUG_PRINT("info",("Server version = '%s'  capabilites: %lu  status: %u  client_flag: %lu",
		     mysql->server_version, mysql->server_capabilities,
		     mysql->server_status, mysql->client_flag));

  /* This needs to be changed as it's not useful with big packets */
  if (mysql->user[0])
    strmake(end, mysql->user, USERNAME_LENGTH);
  else
    read_user_name(end);

  /* We have to handle different version of handshake here */
  DBUG_PRINT("info",("user: %s",end));
  end= strend(end) + 1;
  if (data_len)
  {
    if (mysql->server_capabilities & CLIENT_SECURE_CONNECTION)
    {
      *end++= data_len;
      memcpy(end, data, data_len);
      end+= data_len;
    }
    else
    {
      DBUG_ASSERT(data_len == SCRAMBLE_LENGTH_323 + 1); /* incl. \0 at the end */
      memcpy(end, data, data_len);
      end+= data_len;
    }
  }
  else
    *end++= 0;

  /* Add database if needed */
  if (mpvio->db && (mysql->server_capabilities & CLIENT_CONNECT_WITH_DB))
  {
    end= strmake(end, mpvio->db, NAME_LEN) + 1;
    mysql->db= my_strdup(mpvio->db, MYF(MY_WME));
  }

  if (mysql->server_capabilities & CLIENT_PLUGIN_AUTH)
    end= strmake(end, mpvio->plugin->name, NAME_LEN) + 1;

  /* Write authentication package */
  if (my_net_write(net, (uchar*) buff, (size_t) (end-buff)) || net_flush(net))
  {
    set_mysql_extended_error(mysql, CR_SERVER_LOST, unknown_sqlstate,
                             ER(CR_SERVER_LOST_EXTENDED),
                             "sending authentication information",
                             errno);
    goto error;
  }
  my_afree(buff);
  return 0;
  
error:
  my_afree(buff);
  return 1;
}