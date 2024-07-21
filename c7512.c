add_bwrap (GPtrArray   *array,
	   ScriptExec  *script)
{
  const char * const usrmerged_dirs[] = { "bin", "lib64", "lib", "sbin" };
  int i;

  g_return_val_if_fail (script->outdir != NULL, FALSE);
  g_return_val_if_fail (script->s_infile != NULL, FALSE);

  add_args (array,
	    "bwrap",
	    "--ro-bind", "/usr", "/usr",
	    "--ro-bind", "/etc/ld.so.cache", "/etc/ld.so.cache",
	    NULL);

  /* These directories might be symlinks into /usr/... */
  for (i = 0; i < G_N_ELEMENTS (usrmerged_dirs); i++)
    {
      g_autofree char *absolute_dir = g_strdup_printf ("/%s", usrmerged_dirs[i]);

      if (!g_file_test (absolute_dir, G_FILE_TEST_EXISTS))
        continue;

      if (path_is_usrmerged (absolute_dir))
        {
          g_autofree char *symlink_target = g_strdup_printf ("/usr/%s", absolute_dir);

          add_args (array,
                    "--symlink", symlink_target, absolute_dir,
                    NULL);
        }
      else
        {
          add_args (array,
                    "--ro-bind", absolute_dir, absolute_dir,
                    NULL);
        }
    }

  add_args (array,
	    "--proc", "/proc",
	    "--dev", "/dev",
	    "--chdir", "/",
	    "--setenv", "GIO_USE_VFS", "local",
	    "--unshare-all",
	    "--die-with-parent",
	    NULL);

  add_env (array, "G_MESSAGES_DEBUG");
  add_env (array, "G_MESSAGES_PREFIXED");

  /* Add gnome-desktop's install prefix if needed */
  if (g_strcmp0 (INSTALL_PREFIX, "") != 0 &&
      g_strcmp0 (INSTALL_PREFIX, "/usr") != 0 &&
      g_strcmp0 (INSTALL_PREFIX, "/usr/") != 0)
    {
      add_args (array,
                "--ro-bind", INSTALL_PREFIX, INSTALL_PREFIX,
                NULL);
    }

  g_ptr_array_add (array, g_strdup ("--bind"));
  g_ptr_array_add (array, g_strdup (script->outdir));
  g_ptr_array_add (array, g_strdup ("/tmp"));

  /* We make sure to also re-use the original file's original
   * extension in case it's useful for the thumbnailer to
   * identify the file type */
  g_ptr_array_add (array, g_strdup ("--ro-bind"));
  g_ptr_array_add (array, g_strdup (script->infile));
  g_ptr_array_add (array, g_strdup (script->s_infile));

  return TRUE;
}