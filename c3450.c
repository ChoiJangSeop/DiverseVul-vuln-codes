static int process_options(int argc, char *argv[], char *operation)
{
  int error= 0;
  int i= 0;
  
  /* Parse and execute command-line options */
  if ((error= handle_options(&argc, &argv, my_long_options, get_one_option)))
    goto exit;

  /* If the print defaults option used, exit. */
  if (opt_print_defaults)
  {
    error= -1;
    goto exit;
  }

  /* Add a trailing directory separator if not present */
  if (opt_basedir)
  {
    i= (int)strlength(opt_basedir);
    if (opt_basedir[i-1] != FN_LIBCHAR || opt_basedir[i-1] != FN_LIBCHAR2)
    {
      char buff[FN_REFLEN];
      
      strncpy(buff, opt_basedir, sizeof(buff) - 1);
#ifdef __WIN__
      strncat(buff, "/", sizeof(buff) - strlen(buff) - 1);
#else
      strncat(buff, FN_DIRSEP, sizeof(buff) - strlen(buff) - 1);
#endif
      buff[sizeof(buff) - 1]= 0;
      my_free(opt_basedir);
      opt_basedir= my_strdup(buff, MYF(MY_FAE));
    }
  }
  
  /*
    If the user did not specify the option to skip loading defaults from a
    config file and the required options are not present or there was an error
    generated when the defaults were read from the file, exit.
  */
  if (!opt_no_defaults && ((error= get_default_values())))
  {
    error= -1;
    goto exit;
  }

  /*
   Check to ensure required options are present and validate the operation.
   Note: this method also validates the plugin specified by attempting to
   read a configuration file named <plugin_name>.ini from the --plugin-dir
   or --plugin-ini location if the --plugin-ini option presented.
  */
  strcpy(operation, "");
  if ((error = check_options(argc, argv, operation)))
  {
    goto exit;
  }

  if (opt_verbose)
  {
    printf("#    basedir = %s\n", opt_basedir);
    printf("# plugin_dir = %s\n", opt_plugin_dir);
    printf("#    datadir = %s\n", opt_datadir);
    printf("# plugin_ini = %s\n", opt_plugin_ini);
  }

exit:
  return error;
}