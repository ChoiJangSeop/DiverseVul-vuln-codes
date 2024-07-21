sig_handler handle_sigint(int sig)
{
  char kill_buffer[40];
  MYSQL *kill_mysql= NULL;

  /* terminate if no query being executed, or we already tried interrupting */
  /* terminate if no query being executed, or we already tried interrupting */
  if (!executing_query || (interrupted_query == 2))
  {
    tee_fprintf(stdout, "Ctrl-C -- exit!\n");
    goto err;
  }

  kill_mysql= mysql_init(kill_mysql);
  if (!mysql_connect_ssl_check(kill_mysql, current_host, current_user, opt_password,
                               "", opt_mysql_port, opt_mysql_unix_port, 0,
                               opt_ssl_required))
  {
    tee_fprintf(stdout, "Ctrl-C -- sorry, cannot connect to server to kill query, giving up ...\n");
    goto err;
  }

  interrupted_query++;

  /* mysqld < 5 does not understand KILL QUERY, skip to KILL CONNECTION */
  if ((interrupted_query == 1) && (mysql_get_server_version(&mysql) < 50000))
    interrupted_query= 2;

  /* kill_buffer is always big enough because max length of %lu is 15 */
  sprintf(kill_buffer, "KILL %s%lu",
          (interrupted_query == 1) ? "QUERY " : "",
          mysql_thread_id(&mysql));
  tee_fprintf(stdout, "Ctrl-C -- sending \"%s\" to server ...\n", kill_buffer);
  mysql_real_query(kill_mysql, kill_buffer, (uint) strlen(kill_buffer));
  mysql_close(kill_mysql);
  tee_fprintf(stdout, "Ctrl-C -- query aborted.\n");

  return;

err:
#ifdef _WIN32
  /*
   When SIGINT is raised on Windows, the OS creates a new thread to handle the
   interrupt. Once that thread completes, the main thread continues running 
   only to find that it's resources have already been free'd when the sigint 
   handler called mysql_end(). 
  */
  mysql_thread_end();
  return;
#else
  mysql_end(sig);
#endif  
}