exo_open_launch_desktop_file (const gchar *arg)
{
#ifdef HAVE_GIO_UNIX
  GFile           *gfile;
  gchar           *contents;
  gsize            length;
  gboolean         result;
  GKeyFile        *key_file;
  GDesktopAppInfo *appinfo;

  /* try to open a file from the arguments */
  gfile = g_file_new_for_commandline_arg (arg);
  if (G_UNLIKELY (gfile == NULL))
    return FALSE;

  /* load the contents of the file */
  result = g_file_load_contents (gfile, NULL, &contents, &length, NULL, NULL);
  g_object_unref (G_OBJECT (gfile));
  if (G_UNLIKELY (!result || length == 0))
    return FALSE;

  /* create the key file */
  key_file = g_key_file_new ();
  result = g_key_file_load_from_data (key_file, contents, length, G_KEY_FILE_NONE, NULL);
  g_free (contents);
  if (G_UNLIKELY (!result))
    {
      g_key_file_free (key_file);
      return FALSE;
    }

  /* create the appinfo */
  appinfo = g_desktop_app_info_new_from_keyfile (key_file);
  g_key_file_free (key_file);
  if (G_UNLIKELY (appinfo == NULL))
    return FALSE;

  /* try to launch a (non-hidden) desktop file */
  if (G_LIKELY (!g_desktop_app_info_get_is_hidden (appinfo)))
    result = g_app_info_launch (G_APP_INFO (appinfo), NULL, NULL, NULL);
  else
    result = FALSE;

  g_object_unref (G_OBJECT (appinfo));

#ifndef NDEBUG
  g_debug ("launching desktop file %s", result ? "succeeded" : "failed");
#endif

  return result;
#else /* !HAVE_GIO_UNIX */
  g_critical (_("Launching desktop files is not supported when %s is compiled "
                "without GIO-Unix features."), g_get_prgname ());

  return FALSE;
#endif
}