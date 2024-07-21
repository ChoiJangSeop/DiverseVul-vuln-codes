cockpit_auth_empty_cookie_value (const gchar *path, gboolean secure)
{
  gchar *application = cockpit_auth_parse_application (path, NULL);
  gchar *cookie = application_cookie_name (application);

  /* this is completely security irrelevant, but security scanners complain
   * about the lack of Secure (rhbz#1677767) */
  gchar *cookie_line = g_strdup_printf ("%s=deleted; PATH=/;%s HttpOnly",
                                        cookie,
                                        secure ? " Secure;" : "");

  g_free (application);
  g_free (cookie);

  return cookie_line;
}