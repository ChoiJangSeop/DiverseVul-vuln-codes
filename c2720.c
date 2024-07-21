int STDCALL
mysql_options(MYSQL *mysql,enum mysql_option option, const void *arg)
{
  DBUG_ENTER("mysql_option");
  DBUG_PRINT("enter",("option: %d",(int) option));
  switch (option) {
  case MYSQL_OPT_CONNECT_TIMEOUT:
    mysql->options.connect_timeout= *(uint*) arg;
    break;
  case MYSQL_OPT_READ_TIMEOUT:
    mysql->options.read_timeout= *(uint*) arg;
    break;
  case MYSQL_OPT_WRITE_TIMEOUT:
    mysql->options.write_timeout= *(uint*) arg;
    break;
  case MYSQL_OPT_COMPRESS:
    mysql->options.compress= 1;			/* Remember for connect */
    mysql->options.client_flag|= CLIENT_COMPRESS;
    break;
  case MYSQL_OPT_NAMED_PIPE:			/* This option is depricated */
    mysql->options.protocol=MYSQL_PROTOCOL_PIPE; /* Force named pipe */
    break;
  case MYSQL_OPT_LOCAL_INFILE:			/* Allow LOAD DATA LOCAL ?*/
    if (!arg || test(*(uint*) arg))
      mysql->options.client_flag|= CLIENT_LOCAL_FILES;
    else
      mysql->options.client_flag&= ~CLIENT_LOCAL_FILES;
    break;
  case MYSQL_INIT_COMMAND:
    add_init_command(&mysql->options,arg);
    break;
  case MYSQL_READ_DEFAULT_FILE:
    my_free(mysql->options.my_cnf_file);
    mysql->options.my_cnf_file=my_strdup(key_memory_mysql_options,
                                         arg,MYF(MY_WME));
    break;
  case MYSQL_READ_DEFAULT_GROUP:
    my_free(mysql->options.my_cnf_group);
    mysql->options.my_cnf_group=my_strdup(key_memory_mysql_options,
                                          arg,MYF(MY_WME));
    break;
  case MYSQL_SET_CHARSET_DIR:
    my_free(mysql->options.charset_dir);
    mysql->options.charset_dir=my_strdup(key_memory_mysql_options,
                                         arg,MYF(MY_WME));
    break;
  case MYSQL_SET_CHARSET_NAME:
    my_free(mysql->options.charset_name);
    mysql->options.charset_name=my_strdup(key_memory_mysql_options,
                                          arg,MYF(MY_WME));
    break;
  case MYSQL_OPT_PROTOCOL:
    mysql->options.protocol= *(uint*) arg;
    break;
  case MYSQL_SHARED_MEMORY_BASE_NAME:
#if defined (_WIN32) && !defined (EMBEDDED_LIBRARY)
    if (mysql->options.shared_memory_base_name != def_shared_memory_base_name)
      my_free(mysql->options.shared_memory_base_name);
    mysql->options.shared_memory_base_name=my_strdup(key_memory_mysql_options,
                                                     arg,MYF(MY_WME));
#endif
    break;
  case MYSQL_OPT_USE_REMOTE_CONNECTION:
  case MYSQL_OPT_USE_EMBEDDED_CONNECTION:
  case MYSQL_OPT_GUESS_CONNECTION:
    mysql->options.methods_to_use= option;
    break;
  case MYSQL_SET_CLIENT_IP:
    my_free(mysql->options.ci.client_ip);
    mysql->options.ci.client_ip= my_strdup(key_memory_mysql_options,
                                           arg, MYF(MY_WME));
    break;
  case MYSQL_SECURE_AUTH:
    mysql->options.secure_auth= *(my_bool *) arg;
    break;
  case MYSQL_REPORT_DATA_TRUNCATION:
    mysql->options.report_data_truncation= test(*(my_bool *) arg);
    break;
  case MYSQL_OPT_RECONNECT:
    mysql->reconnect= *(my_bool *) arg;
    break;
  case MYSQL_OPT_BIND:
    my_free(mysql->options.ci.bind_address);
    mysql->options.ci.bind_address= my_strdup(key_memory_mysql_options,
                                              arg, MYF(MY_WME));
    break;
  case MYSQL_OPT_SSL_VERIFY_SERVER_CERT:
    if (*(my_bool*) arg)
      mysql->options.client_flag|= CLIENT_SSL_VERIFY_SERVER_CERT;
    else
      mysql->options.client_flag&= ~CLIENT_SSL_VERIFY_SERVER_CERT;
    break;
  case MYSQL_PLUGIN_DIR:
    EXTENSION_SET_STRING(&mysql->options, plugin_dir, arg);
    break;
  case MYSQL_DEFAULT_AUTH:
    EXTENSION_SET_STRING(&mysql->options, default_auth, arg);
    break;
  case MYSQL_OPT_SSL_KEY:      SET_SSL_OPTION(ssl_key, arg);     break;
  case MYSQL_OPT_SSL_CERT:     SET_SSL_OPTION(ssl_cert, arg);    break;
  case MYSQL_OPT_SSL_CA:       SET_SSL_OPTION(ssl_ca, arg);      break;
  case MYSQL_OPT_SSL_CAPATH:   SET_SSL_OPTION(ssl_capath, arg);  break;
  case MYSQL_OPT_SSL_CIPHER:   SET_SSL_OPTION(ssl_cipher, arg);  break;
  case MYSQL_OPT_SSL_CRL:      EXTENSION_SET_SSL_STRING(&mysql->options,
                                                        ssl_crl, arg);
                               break;
  case MYSQL_OPT_SSL_CRLPATH:  EXTENSION_SET_SSL_STRING(&mysql->options,
                                                        ssl_crlpath, arg);
                               break;
  case MYSQL_SERVER_PUBLIC_KEY:
    EXTENSION_SET_STRING(&mysql->options, server_public_key_path, arg);
    break;

  case MYSQL_OPT_CONNECT_ATTR_RESET:
    ENSURE_EXTENSIONS_PRESENT(&mysql->options);
    if (my_hash_inited(&mysql->options.extension->connection_attributes))
    {
      my_hash_free(&mysql->options.extension->connection_attributes);
      mysql->options.extension->connection_attributes_length= 0;
    }
    break;
  case MYSQL_OPT_CONNECT_ATTR_DELETE:
    ENSURE_EXTENSIONS_PRESENT(&mysql->options);
    if (my_hash_inited(&mysql->options.extension->connection_attributes))
    {
      size_t len;
      uchar *elt;

      len= arg ? strlen(arg) : 0;

      if (len)
      {
        elt= my_hash_search(&mysql->options.extension->connection_attributes,
                            arg, len);
        if (elt)
        {
          LEX_STRING *attr= (LEX_STRING *) elt;
          LEX_STRING *key= attr, *value= attr + 1;

          mysql->options.extension->connection_attributes_length-=
            get_length_store_length(key->length) + key->length +
            get_length_store_length(value->length) + value->length;

          my_hash_delete(&mysql->options.extension->connection_attributes,
                         elt);

        }
      }
    }
    break;
  case MYSQL_ENABLE_CLEARTEXT_PLUGIN:
    ENSURE_EXTENSIONS_PRESENT(&mysql->options);
    mysql->options.extension->enable_cleartext_plugin= 
      (*(my_bool*) arg) ? TRUE : FALSE;
    break;
  case MYSQL_OPT_CAN_HANDLE_EXPIRED_PASSWORDS:
    if (*(my_bool*) arg)
      mysql->options.client_flag|= CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS;
    else
      mysql->options.client_flag&= ~CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS;
    break;

  default:
    DBUG_RETURN(1);
  }
  DBUG_RETURN(0);