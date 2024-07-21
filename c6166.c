mysql_real_connect(MYSQL *mysql,const char *host, const char *user,
		   const char *passwd, const char *db,
		   uint port, const char *unix_socket,ulong client_flag)
{
  char name_buff[USERNAME_LENGTH];

  DBUG_ENTER("mysql_real_connect");
  DBUG_PRINT("enter",("host: %s  db: %s  user: %s (libmysqld)",
		      host ? host : "(Null)",
		      db ? db : "(Null)",
		      user ? user : "(Null)"));

  /* Test whether we're already connected */
  if (mysql->server_version)
  {
    set_mysql_error(mysql, CR_ALREADY_CONNECTED, unknown_sqlstate);
    DBUG_RETURN(0);
  }

  if (!host || !host[0])
    host= mysql->options.host;

  if (mysql->options.methods_to_use == MYSQL_OPT_USE_REMOTE_CONNECTION ||
      (mysql->options.methods_to_use == MYSQL_OPT_GUESS_CONNECTION &&
       host && *host && strcmp(host,LOCAL_HOST)))
    DBUG_RETURN(cli_mysql_real_connect(mysql, host, user, 
				       passwd, db, port, 
				       unix_socket, client_flag));

  mysql->methods= &embedded_methods;

  /* use default options */
  if (mysql->options.my_cnf_file || mysql->options.my_cnf_group)
  {
    mysql_read_default_options(&mysql->options,
			       (mysql->options.my_cnf_file ?
				mysql->options.my_cnf_file : "my"),
			       mysql->options.my_cnf_group);
    my_free(mysql->options.my_cnf_file);
    my_free(mysql->options.my_cnf_group);
    mysql->options.my_cnf_file=mysql->options.my_cnf_group=0;
  }

  if (!db || !db[0])
    db=mysql->options.db;

  if (!user || !user[0])
    user=mysql->options.user;

#ifndef NO_EMBEDDED_ACCESS_CHECKS
  if (!passwd)
  {
    passwd=mysql->options.password;
#if !defined(DONT_USE_MYSQL_PWD)
    if (!passwd)
      passwd=getenv("MYSQL_PWD");		/* get it from environment */
#endif
  }
  mysql->passwd= passwd ? my_strdup(passwd,MYF(0)) : NULL;
#endif /*!NO_EMBEDDED_ACCESS_CHECKS*/
  if (!user || !user[0])
  {
    read_user_name(name_buff);
    if (name_buff[0])
      user= name_buff;
  }

  if (!user)
    user= "";
   /* 
      We need to alloc some space for mysql->info but don't want to
      put extra 'my_free's in mysql_close.
      So we alloc it with the 'user' string to be freed at once
   */
  mysql->user= my_strdup(user, MYF(0));

  port=0;
  unix_socket=0;

  client_flag|=mysql->options.client_flag;
  /* Send client information for access check */
  client_flag|=CLIENT_CAPABILITIES;
  if (client_flag & CLIENT_MULTI_STATEMENTS)
    client_flag|= CLIENT_MULTI_RESULTS;
  /*
    no compression in embedded as we don't send any data,
    and no pluggable auth, as we cannot do a client-server dialog
  */
  client_flag&= ~(CLIENT_COMPRESS | CLIENT_PLUGIN_AUTH);
  if (db)
    client_flag|=CLIENT_CONNECT_WITH_DB;

  mysql->info_buffer= my_malloc(MYSQL_ERRMSG_SIZE, MYF(0));
  mysql->thd= create_embedded_thd(client_flag);

  init_embedded_mysql(mysql, client_flag);

  if (mysql_init_character_set(mysql))
    goto error;

  if (check_embedded_connection(mysql, db))
    goto error;

  mysql->server_status= SERVER_STATUS_AUTOCOMMIT;

  if (mysql->options.init_commands)
  {
    DYNAMIC_ARRAY *init_commands= mysql->options.init_commands;
    char **ptr= (char**)init_commands->buffer;
    char **end= ptr + init_commands->elements;

    for (; ptr<end; ptr++)
    {
      MYSQL_RES *res;
      if (mysql_query(mysql,*ptr))
	goto error;
      if (mysql->fields)
      {
	if (!(res= (*mysql->methods->use_result)(mysql)))
	  goto error;
	mysql_free_result(res);
      }
    }
  }

  DBUG_PRINT("exit",("Mysql handler: 0x%lx", (long) mysql));
  DBUG_RETURN(mysql);

error:
  DBUG_PRINT("error",("message: %u (%s)",
                      mysql->net.last_errno,
                      mysql->net.last_error));
  {
    /* Free alloced memory */
    my_bool free_me=mysql->free_me;
    free_old_query(mysql); 
    mysql->free_me=0;
    mysql_close(mysql);
    mysql->free_me=free_me;
  }
  DBUG_RETURN(0);
}