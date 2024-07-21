int connect_n_handle_errors(struct st_command *command,
                            MYSQL* con, const char* host,
                            const char* user, const char* pass,
                            const char* db, int port, const char* sock)
{
  DYNAMIC_STRING *ds;
  int failed_attempts= 0;

  ds= &ds_res;

  /* Only log if an error is expected */
  if (command->expected_errors.count > 0 &&
      !disable_query_log)
  {
    /*
      Log the connect to result log
    */
    dynstr_append_mem(ds, "connect(", 8);
    replace_dynstr_append(ds, host);
    dynstr_append_mem(ds, ",", 1);
    replace_dynstr_append(ds, user);
    dynstr_append_mem(ds, ",", 1);
    replace_dynstr_append(ds, pass);
    dynstr_append_mem(ds, ",", 1);
    if (db)
      replace_dynstr_append(ds, db);
    dynstr_append_mem(ds, ",", 1);
    replace_dynstr_append_uint(ds, port);
    dynstr_append_mem(ds, ",", 1);
    if (sock)
      replace_dynstr_append(ds, sock);
    dynstr_append_mem(ds, ")", 1);
    dynstr_append_mem(ds, delimiter, delimiter_length);
    dynstr_append_mem(ds, "\n", 1);
  }
  /* Simlified logging if enabled */
  if (!disable_connect_log && !disable_query_log)
  {
    replace_dynstr_append(ds, command->query);
    dynstr_append_mem(ds, ";\n", 2);
  }
  
  mysql_options(con, MYSQL_OPT_CONNECT_ATTR_RESET, 0);
  mysql_options4(con, MYSQL_OPT_CONNECT_ATTR_ADD, "program_name", "mysqltest");
  mysql_options(con, MYSQL_OPT_CAN_HANDLE_EXPIRED_PASSWORDS,
                &can_handle_expired_passwords);
  while (!mysql_connect_ssl_check(con, host, user, pass, db, port,
                                  sock ? sock: 0, CLIENT_MULTI_STATEMENTS,
                                  opt_ssl_required))
  {
    /*
      If we have used up all our connections check whether this
      is expected (by --error). If so, handle the error right away.
      Otherwise, give it some extra time to rule out race-conditions.
      If extra-time doesn't help, we have an unexpected error and
      must abort -- just proceeding to handle_error() when second
      and third chances are used up will handle that for us.

      There are various user-limits of which only max_user_connections
      and max_connections_per_hour apply at connect time. For the
      the second to create a race in our logic, we'd need a limits
      test that runs without a FLUSH for longer than an hour, so we'll
      stay clear of trying to work out which exact user-limit was
      exceeded.
    */

    if (((mysql_errno(con) == ER_TOO_MANY_USER_CONNECTIONS) ||
         (mysql_errno(con) == ER_USER_LIMIT_REACHED)) &&
        (failed_attempts++ < opt_max_connect_retries))
    {
      int i;

      i= match_expected_error(command, mysql_errno(con), mysql_sqlstate(con));

      if (i >= 0)
        goto do_handle_error;                 /* expected error, handle */

      my_sleep(connection_retry_sleep);       /* unexpected error, wait */
      continue;                               /* and give it 1 more chance */
    }

do_handle_error:
    var_set_errno(mysql_errno(con));
    handle_error(command, mysql_errno(con), mysql_error(con),
		 mysql_sqlstate(con), ds);
    return 0; /* Not connected */
  }

  var_set_errno(0);
  handle_no_error(command);
  revert_properties();
  return 1; /* Connected */
}