static my_bool sql_connect(MYSQL *mysql, uint wait)
{
  my_bool info=0;

  for (;;)
  {
    if (mysql_connect_ssl_check(mysql, host, user, opt_password, NullS,
                                tcp_port, unix_port,
                                CLIENT_REMEMBER_OPTIONS, opt_ssl_required))
    {
      mysql->reconnect= 1;
      if (info)
      {
	fputs("\n",stderr);
	(void) fflush(stderr);
      }
      return 0;
    }

    if (!wait)                                  // was or reached 0, fail
    {
      if (!option_silent)                       // print diagnostics
      {
	if (!host)
	  host= (char*) LOCAL_HOST;
	my_printf_error(0,"connect to server at '%s' failed\nerror: '%s'",
			error_flags, host, mysql_error(mysql));
	if (mysql_errno(mysql) == CR_CONNECTION_ERROR)
	{
	  fprintf(stderr,
		  "Check that mysqld is running and that the socket: '%s' exists!\n",
		  unix_port ? unix_port : mysql_unix_port);
	}
	else if (mysql_errno(mysql) == CR_CONN_HOST_ERROR ||
		 mysql_errno(mysql) == CR_UNKNOWN_HOST)
	{
	  fprintf(stderr,"Check that mysqld is running on %s",host);
	  fprintf(stderr," and that the port is %d.\n",
		  tcp_port ? tcp_port: mysql_port);
	  fprintf(stderr,"You can check this by doing 'telnet %s %d'\n",
		  host, tcp_port ? tcp_port: mysql_port);
	}
      }
      return 1;
    }

    if (wait != (uint) ~0)
      wait--;				/* count down, one less retry */

    if ((mysql_errno(mysql) != CR_CONN_HOST_ERROR) &&
	(mysql_errno(mysql) != CR_CONNECTION_ERROR))
    {
      /*
        Error is worse than "server doesn't answer (yet?)";
        fail even if we still have "wait-coins" unless --force
        was also given.
      */
      fprintf(stderr,"Got error: %s\n", mysql_error(mysql));
      if (!option_force)
	return 1;
    }
    else if (!option_silent)
    {
      if (!info)
      {
	info=1;
	fputs("Waiting for MySQL server to answer",stderr);
	(void) fflush(stderr);
      }
      else
      {
	putc('.',stderr);
	(void) fflush(stderr);
      }
    }
    sleep(5);
  }
}