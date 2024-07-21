com_connect(String *buffer, char *line)
{
  char *tmp, buff[256];
  my_bool save_rehash= opt_rehash;
  int error;

  bzero(buff, sizeof(buff));
  if (buffer)
  {
    /*
      Two null bytes are needed in the end of buff to allow
      get_arg to find end of string the second time it's called.
    */
    tmp= strmake(buff, line, sizeof(buff)-2);
#ifdef EXTRA_DEBUG
    tmp[1]= 0;
#endif
    tmp= get_arg(buff, 0);
    if (tmp && *tmp)
    {
      my_free(current_db);
      current_db= my_strdup(tmp, MYF(MY_WME));
      tmp= get_arg(buff, 1);
      if (tmp)
      {
	my_free(current_host);
	current_host=my_strdup(tmp,MYF(MY_WME));
      }
    }
    else
    {
      /* Quick re-connect */
      opt_rehash= 0;                            /* purecov: tested */
    }
    buffer->length(0);				// command used
  }
  else
    opt_rehash= 0;
  error=sql_connect(current_host,current_db,current_user,opt_password,0);
  opt_rehash= save_rehash;

  if (connected)
  {
    sprintf(buff,"Connection id:    %lu",mysql_thread_id(&mysql));
    put_info(buff,INFO_INFO);
    sprintf(buff,"Current database: %.128s\n",
	    current_db ? current_db : "*** NONE ***");
    put_info(buff,INFO_INFO);
  }
  return error;
}