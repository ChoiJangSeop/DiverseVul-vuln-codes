void do_exec(struct st_command *command)
{
  int error;
  char buf[512];
  FILE *res_file;
  char *cmd= command->first_argument;
  DYNAMIC_STRING ds_cmd;
  DYNAMIC_STRING ds_sorted, *ds_result;
  DBUG_ENTER("do_exec");
  DBUG_PRINT("enter", ("cmd: '%s'", cmd));

  /* Skip leading space */
  while (*cmd && my_isspace(charset_info, *cmd))
    cmd++;
  if (!*cmd)
  {
    report_or_die("Missing argument in exec");
    return;
  }
  command->last_argument= command->end;

  init_dynamic_string(&ds_cmd, 0, command->query_len+256, 256);
  /* Eval the command, thus replacing all environment variables */
  do_eval(&ds_cmd, cmd, command->end, !is_windows);

  /* Check if echo should be replaced with "builtin" echo */
  if (builtin_echo[0] && strncmp(cmd, "echo", 4) == 0)
  {
    /* Replace echo with our "builtin" echo */
    replace(&ds_cmd, "echo", 4, builtin_echo, strlen(builtin_echo));
  }

#ifdef __WIN__
#ifndef USE_CYGWIN
  /* Replace /dev/null with NUL */
  while(replace(&ds_cmd, "/dev/null", 9, "NUL", 3) == 0)
    ;
  /* Replace "closed stdout" with non existing output fd */
  while(replace(&ds_cmd, ">&-", 3, ">&4", 3) == 0)
    ;
#endif
#endif

  /* exec command is interpreted externally and will not take newlines */
  while(replace(&ds_cmd, "\n", 1, " ", 1) == 0)
    ;
  
  DBUG_PRINT("info", ("Executing '%s' as '%s'",
                      command->first_argument, ds_cmd.str));

  if (!(res_file= my_popen(&ds_cmd, "r")))
  {
    dynstr_free(&ds_cmd);
    if (command->abort_on_error)
      report_or_die("popen(\"%s\", \"r\") failed", command->first_argument);
    return;
  }

  ds_result= &ds_res;
  if (display_result_sorted)
  {
    init_dynamic_string(&ds_sorted, "", 1024, 1024);
    ds_result= &ds_sorted;
  }

  while (fgets(buf, sizeof(buf), res_file))
  {
    if (disable_result_log)
    {
      buf[strlen(buf)-1]=0;
      DBUG_PRINT("exec_result",("%s", buf));
    }
    else
    {
      replace_dynstr_append(ds_result, buf);
    }
  }
  error= pclose(res_file);

  if (display_result_sorted)
  {
    dynstr_append_sorted(&ds_res, &ds_sorted, 0);
    dynstr_free(&ds_sorted);
  }

  if (error > 0)
  {
    uint status= WEXITSTATUS(error);
    int i;

    if (command->abort_on_error)
    {
      report_or_die("exec of '%s' failed, error: %d, status: %d, errno: %d\n"
                    "Output from before failure:\n%s\n",
                    ds_cmd.str, error, status, errno,
                    ds_res.str);
      dynstr_free(&ds_cmd);
      return;
    }

    DBUG_PRINT("info",
               ("error: %d, status: %d", error, status));

    i= match_expected_error(command, status, NULL);

    if (i >= 0)
      DBUG_PRINT("info", ("command \"%s\" failed with expected error: %d",
                          command->first_argument, status));
    else
    {
      dynstr_free(&ds_cmd);
      if (command->expected_errors.count > 0)
        report_or_die("command \"%s\" failed with wrong error: %d",
                      command->first_argument, status);
    }
  }
  else if (command->expected_errors.err[0].type == ERR_ERRNO &&
           command->expected_errors.err[0].code.errnum != 0)
  {
    /* Error code we wanted was != 0, i.e. not an expected success */
    log_msg("exec of '%s failed, error: %d, errno: %d",
            ds_cmd.str, error, errno);
    dynstr_free(&ds_cmd);
    report_or_die("command \"%s\" succeeded - should have failed with "
                  "errno %d...",
                  command->first_argument,
                  command->expected_errors.err[0].code.errnum);
  }

  dynstr_free(&ds_cmd);
  DBUG_VOID_RETURN;
}