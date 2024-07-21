ephy_embed_single_initialize (EphyEmbedSingle *single)
{
  SoupSession *session;
  SoupCookieJar *jar;
  char *filename;
  char *cookie_policy;

  /* Initialise nspluginwrapper's plugins if available */
  if (g_file_test (NSPLUGINWRAPPER_SETUP, G_FILE_TEST_EXISTS) != FALSE)
    g_spawn_command_line_sync (NSPLUGINWRAPPER_SETUP, NULL, NULL, NULL, NULL);

  ephy_embed_prefs_init ();

  session = webkit_get_default_session ();

#ifdef GTLS_SYSTEM_CA_FILE
  /* Check SSL certificates */

  if (g_file_test (GTLS_SYSTEM_CA_FILE, G_FILE_TEST_EXISTS)) {
    g_object_set (session,
                  SOUP_SESSION_SSL_CA_FILE, GTLS_SYSTEM_CA_FILE,
                  "ignore-ssl-cert-errors", TRUE,
                  NULL);
  } else {
    g_warning (_("CA Certificates file we should use was not found, "\
                 "all SSL sites will be considered to have a broken certificate."));
  }
#endif

  /* Store cookies in moz-compatible SQLite format */
  filename = g_build_filename (ephy_dot_dir (), "cookies.sqlite", NULL);
  jar = soup_cookie_jar_sqlite_new (filename, FALSE);
  g_free (filename);
  cookie_policy = eel_gconf_get_string (CONF_SECURITY_COOKIES_ACCEPT);
  ephy_embed_prefs_set_cookie_jar_policy (jar, cookie_policy);
  g_free (cookie_policy);

  soup_session_add_feature (session, SOUP_SESSION_FEATURE (jar));
  g_object_unref (jar);

  /* Use GNOME proxy settings through libproxy */
  soup_session_add_feature_by_type (session, SOUP_TYPE_PROXY_RESOLVER_GNOME);

#ifdef SOUP_TYPE_PASSWORD_MANAGER
  /* Use GNOME keyring to store passwords. Only add the manager if we
     are not using a private session, otherwise we want any new
     password to expire when we exit *and* we don't want to use any
     existing password in the keyring */
  if (ephy_has_private_profile () == FALSE)
    soup_session_add_feature_by_type (session, SOUP_TYPE_PASSWORD_MANAGER_GNOME);
#endif

  return TRUE;
}