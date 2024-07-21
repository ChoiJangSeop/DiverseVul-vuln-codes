static int run_tool(char *tool_path, DYNAMIC_STRING *ds_res, ...)
{
  int ret;
  const char* arg;
  va_list args;
  DYNAMIC_STRING ds_cmdline;

  DBUG_ENTER("run_tool");
  DBUG_PRINT("enter", ("tool_path: %s", tool_path));

  if (init_dynamic_string(&ds_cmdline, IF_WIN("\"", ""), FN_REFLEN, FN_REFLEN))
    die("Out of memory");

  dynstr_append_os_quoted(&ds_cmdline, tool_path, NullS);
  dynstr_append(&ds_cmdline, " ");

  va_start(args, ds_res);

  while ((arg= va_arg(args, char *)))
  {
    /* Options should be os quoted */
    if (strncmp(arg, "--", 2) == 0)
      dynstr_append_os_quoted(&ds_cmdline, arg, NullS);
    else
      dynstr_append(&ds_cmdline, arg);
    dynstr_append(&ds_cmdline, " ");
  }

  va_end(args);

  /* If given --ssl-mode=REQUIRED propagate it to the tool. */
  if (opt_ssl_required)
    dynstr_append(&ds_cmdline, "--ssl-mode=REQUIRED");

#ifdef __WIN__
  dynstr_append(&ds_cmdline, "\"");
#endif

  DBUG_PRINT("info", ("Running: %s", ds_cmdline.str));
  ret= run_command(ds_cmdline.str, ds_res);
  DBUG_PRINT("exit", ("ret: %d", ret));
  dynstr_free(&ds_cmdline);
  DBUG_RETURN(ret);
}