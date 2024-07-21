*/
static int send_client_reply_packet(MCPVIO_EXT *mpvio,
                                    const uchar *data, int data_len)
{
  MYSQL *mysql= mpvio->mysql;
  NET *net= &mysql->net;
  char *buff, *end;
  size_t buff_size;
  size_t connect_attrs_len=
    (mysql->server_capabilities & CLIENT_CONNECT_ATTRS &&
     mysql->options.extension) ?
    mysql->options.extension->connection_attributes_length : 0;

  DBUG_ASSERT(connect_attrs_len < MAX_CONNECTION_ATTR_STORAGE_LENGTH);

  
  /*
    see end= buff+32 below, fixed size of the packet is 32 bytes.
     +9 because data is a length encoded binary where meta data size is max 9.
  */
  buff_size= 33 + USERNAME_LENGTH + data_len + 9 + NAME_LEN + NAME_LEN + connect_attrs_len + 9;
  buff= my_alloca(buff_size);

  mysql->client_flag|= mysql->options.client_flag;
  mysql->client_flag|= CLIENT_CAPABILITIES;

  if (mysql->client_flag & CLIENT_MULTI_STATEMENTS)
    mysql->client_flag|= CLIENT_MULTI_RESULTS;

#if defined(HAVE_OPENSSL) && !defined(EMBEDDED_LIBRARY)
  if (mysql->options.ssl_key || mysql->options.ssl_cert ||
      mysql->options.ssl_ca || mysql->options.ssl_capath ||
      mysql->options.ssl_cipher ||
      (mysql->options.extension && mysql->options.extension->ssl_crl) || 
      (mysql->options.extension && mysql->options.extension->ssl_crlpath))
    mysql->options.use_ssl= 1;
  if (mysql->options.use_ssl)
    mysql->client_flag|= CLIENT_SSL;
#endif /* HAVE_OPENSSL && !EMBEDDED_LIBRARY*/
  if (mpvio->db)
    mysql->client_flag|= CLIENT_CONNECT_WITH_DB;
  else
    mysql->client_flag&= ~CLIENT_CONNECT_WITH_DB;

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
    memset(buff+9, 0, 32-9);
    end= buff+32;
  }
  else
  {
    int2store(buff, mysql->client_flag);
    int3store(buff+2, net->max_packet_size);
    end= buff+5;
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
    MYSQL_TRACE(SEND_SSL_REQUEST, mysql, (end - buff, (const unsigned char*)buff));
    if (my_net_write(net, (uchar*)buff, (size_t) (end-buff)) || net_flush(net))
    {
      set_mysql_extended_error(mysql, CR_SERVER_LOST, unknown_sqlstate,
                               ER(CR_SERVER_LOST_EXTENDED),
                               "sending connection information to server",
                               errno);
      goto error;
    }

    MYSQL_TRACE_STAGE(mysql, SSL_NEGOTIATION);

    /* Create the VioSSLConnectorFd - init SSL and load certs */
    if (!(ssl_fd= new_VioSSLConnectorFd(options->ssl_key,
                                        options->ssl_cert,
                                        options->ssl_ca,
                                        options->ssl_capath,
                                        options->ssl_cipher,
                                        &ssl_init_error,
                                        options->extension ? 
                                        options->extension->ssl_crl : NULL,
                                        options->extension ? 
                                        options->extension->ssl_crlpath : NULL)))
    {
      set_mysql_extended_error(mysql, CR_SSL_CONNECTION_ERROR, unknown_sqlstate,
                               ER(CR_SSL_CONNECTION_ERROR), sslGetErrString(ssl_init_error));
      goto error;
    }
    mysql->connector_fd= (unsigned char *) ssl_fd;

    /* Connect to the server */
    DBUG_PRINT("info", ("IO layer change in progress..."));
    MYSQL_TRACE(SSL_CONNECT, mysql, ());
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

    MYSQL_TRACE(SSL_CONNECTED, mysql, ());
    MYSQL_TRACE_STAGE(mysql, AUTHENTICATE);
  }
#endif /* HAVE_OPENSSL */

  DBUG_PRINT("info",("Server version = '%s'  capabilites: %lu  status: %u  client_flag: %lu",
		     mysql->server_version, mysql->server_capabilities,
		     mysql->server_status, mysql->client_flag));

  compile_time_assert(MYSQL_USERNAME_LENGTH == USERNAME_LENGTH);

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
      /* 
        Since the older versions of server do not have
        CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA capability,
        a check is performed on this before sending auth data.
        If lenenc support is not available, the data is sent
        in the format of first byte representing the length of
        the string followed by the actual string.
      */
      if (mysql->server_capabilities & CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA)
        end= write_length_encoded_string4(end, (char *)(buff + buff_size),
                                         (char *) data,
                                         (char *)(data + data_len));
      else
        end= write_string(end, (char *)(buff + buff_size),
                         (char *) data,
                         (char *)(data + data_len));
      if (end == NULL)
        goto error;
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
    mysql->db= my_strdup(key_memory_MYSQL,
                         mpvio->db, MYF(MY_WME));
  }

  if (mysql->server_capabilities & CLIENT_PLUGIN_AUTH)
    end= strmake(end, mpvio->plugin->name, NAME_LEN) + 1;

  end= (char *) send_client_connect_attrs(mysql, (uchar *) end);

  /* Write authentication package */
  MYSQL_TRACE(SEND_AUTH_RESPONSE, mysql, (end-buff, (const unsigned char*)buff));
  if (my_net_write(net, (uchar*) buff, (size_t) (end-buff)) || net_flush(net))
  {
    set_mysql_extended_error(mysql, CR_SERVER_LOST, unknown_sqlstate,
                             ER(CR_SERVER_LOST_EXTENDED),
                             "sending authentication information",
                             errno);
    goto error;
  }
  MYSQL_TRACE(PACKET_SENT, mysql, (end-buff));
  my_afree(buff);
  return 0;
  
error:
  my_afree(buff);
  return 1;