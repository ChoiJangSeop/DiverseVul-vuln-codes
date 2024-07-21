static int check_options(int argc, char **argv, char *operation)
{
  int i= 0;                    // loop counter
  int num_found= 0;            // number of options found (shortcut loop)
  char config_file[FN_REFLEN]; // configuration file name
  char plugin_name[FN_REFLEN]; // plugin name
  
  /* Form prefix strings for the options. */
  const char *basedir_prefix = "--basedir=";
  int basedir_len= strlen(basedir_prefix);
  const char *datadir_prefix = "--datadir=";
  int datadir_len= strlen(datadir_prefix);
  const char *plugin_dir_prefix = "--plugin_dir=";
  int plugin_dir_len= strlen(plugin_dir_prefix);

  strcpy(plugin_name, "");
  for (i = 0; i < argc && num_found < 5; i++)
  {

    if (!argv[i])
    {
      continue;
    }
    if ((strcasecmp(argv[i], "ENABLE") == 0) ||
        (strcasecmp(argv[i], "DISABLE") == 0))
    {
      strcpy(operation, argv[i]);
      num_found++;
    }
    else if ((strncasecmp(argv[i], basedir_prefix, basedir_len) == 0) &&
             !opt_basedir)
    {
      opt_basedir= my_strndup(argv[i]+basedir_len,
                              strlen(argv[i])-basedir_len, MYF(MY_FAE));
      num_found++;
    }
    else if ((strncasecmp(argv[i], datadir_prefix, datadir_len) == 0) &&
             !opt_datadir)
    {
      opt_datadir= my_strndup(argv[i]+datadir_len,
                              strlen(argv[i])-datadir_len, MYF(MY_FAE));
      num_found++;
    }
    else if ((strncasecmp(argv[i], plugin_dir_prefix, plugin_dir_len) == 0) &&
             !opt_plugin_dir)
    {
      opt_plugin_dir= my_strndup(argv[i]+plugin_dir_len,
                                 strlen(argv[i])-plugin_dir_len, MYF(MY_FAE));
      num_found++;
    }
    /* read the plugin config file and check for match against argument */
    else
    {
      strcpy(plugin_name, argv[i]);
      strcpy(config_file, argv[i]);
      strcat(config_file, ".ini");
    }
  }

  if (!opt_basedir)
  {
    fprintf(stderr, "ERROR: Missing --basedir option.\n");
    return 1;
  }

  if (!opt_datadir)
  {
    fprintf(stderr, "ERROR: Missing --datadir option.\n");
    return 1;
  }

  if (!opt_plugin_dir)
  {
    fprintf(stderr, "ERROR: Missing --plugin_dir option.\n");
    return 1;
  }
  /* If a plugin was specified, read the config file. */
  else if (strlen(plugin_name) > 0) 
  {
    if (load_plugin_data(plugin_name, config_file))
    {
      return 1;
    }
    if (strcasecmp(plugin_data.name, plugin_name) != 0)
    {
      fprintf(stderr, "ERROR: plugin name requested does not match config "
              "file data.\n");
      return 1;
    }
  }
  else
  {
    fprintf(stderr, "ERROR: No plugin specified.\n");
    return 1;
  }

  if ((strlen(operation) == 0))
  {
    fprintf(stderr, "ERROR: missing operation. Please specify either "
            "'<plugin> ENABLE' or '<plugin> DISABLE'.\n");
    return 1;
  }

  return 0;
}