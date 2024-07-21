test_full_context (void)
{
  g_autoptr(FlatpakBwrap) bwrap = flatpak_bwrap_new (NULL);
  g_autoptr(FlatpakContext) context = flatpak_context_new ();
  g_autoptr(FlatpakExports) exports = NULL;
  g_autoptr(GError) error = NULL;
  g_autoptr(GKeyFile) keyfile = g_key_file_new ();
  g_autofree gchar *text = NULL;
  g_auto(GStrv) strv = NULL;
  gsize i, n;

  g_key_file_set_value (keyfile,
                        FLATPAK_METADATA_GROUP_CONTEXT,
                        FLATPAK_METADATA_KEY_SHARED,
                        "network;ipc;");
  g_key_file_set_value (keyfile,
                        FLATPAK_METADATA_GROUP_CONTEXT,
                        FLATPAK_METADATA_KEY_SOCKETS,
                        "x11;wayland;pulseaudio;session-bus;system-bus;"
                        "fallback-x11;ssh-auth;pcsc;cups;");
  g_key_file_set_value (keyfile,
                        FLATPAK_METADATA_GROUP_CONTEXT,
                        FLATPAK_METADATA_KEY_DEVICES,
                        "dri;all;kvm;shm;");
  g_key_file_set_value (keyfile,
                        FLATPAK_METADATA_GROUP_CONTEXT,
                        FLATPAK_METADATA_KEY_FEATURES,
                        "devel;multiarch;bluetooth;canbus;");
  g_key_file_set_value (keyfile,
                        FLATPAK_METADATA_GROUP_CONTEXT,
                        FLATPAK_METADATA_KEY_FILESYSTEMS,
                        "host;/home;!/opt");
  g_key_file_set_value (keyfile,
                        FLATPAK_METADATA_GROUP_CONTEXT,
                        FLATPAK_METADATA_KEY_PERSISTENT,
                        ".openarena;");
  g_key_file_set_value (keyfile,
                        FLATPAK_METADATA_GROUP_SESSION_BUS_POLICY,
                        "org.example.SessionService",
                        "own");
  g_key_file_set_value (keyfile,
                        FLATPAK_METADATA_GROUP_SYSTEM_BUS_POLICY,
                        "net.example.SystemService",
                        "talk");
  g_key_file_set_value (keyfile,
                        FLATPAK_METADATA_GROUP_ENVIRONMENT,
                        "HYPOTHETICAL_PATH", "/foo:/bar");
  g_key_file_set_value (keyfile,
                        FLATPAK_METADATA_GROUP_PREFIX_POLICY "MyPolicy",
                        "Colours", "blue;green;");

  flatpak_context_load_metadata (context, keyfile, &error);
  g_assert_no_error (error);

  g_assert_cmpuint (context->shares, ==,
                    (FLATPAK_CONTEXT_SHARED_NETWORK |
                     FLATPAK_CONTEXT_SHARED_IPC));
  g_assert_cmpuint (context->shares_valid, ==, context->shares);
  g_assert_cmpuint (context->devices, ==,
                    (FLATPAK_CONTEXT_DEVICE_DRI |
                     FLATPAK_CONTEXT_DEVICE_ALL |
                     FLATPAK_CONTEXT_DEVICE_KVM |
                     FLATPAK_CONTEXT_DEVICE_SHM));
  g_assert_cmpuint (context->devices_valid, ==, context->devices);
  g_assert_cmpuint (context->sockets, ==,
                    (FLATPAK_CONTEXT_SOCKET_X11 |
                     FLATPAK_CONTEXT_SOCKET_WAYLAND |
                     FLATPAK_CONTEXT_SOCKET_PULSEAUDIO |
                     FLATPAK_CONTEXT_SOCKET_SESSION_BUS |
                     FLATPAK_CONTEXT_SOCKET_SYSTEM_BUS |
                     FLATPAK_CONTEXT_SOCKET_FALLBACK_X11 |
                     FLATPAK_CONTEXT_SOCKET_SSH_AUTH |
                     FLATPAK_CONTEXT_SOCKET_PCSC |
                     FLATPAK_CONTEXT_SOCKET_CUPS));
  g_assert_cmpuint (context->sockets_valid, ==, context->sockets);
  g_assert_cmpuint (context->features, ==,
                    (FLATPAK_CONTEXT_FEATURE_DEVEL |
                     FLATPAK_CONTEXT_FEATURE_MULTIARCH |
                     FLATPAK_CONTEXT_FEATURE_BLUETOOTH |
                     FLATPAK_CONTEXT_FEATURE_CANBUS));
  g_assert_cmpuint (context->features_valid, ==, context->features);

  g_assert_cmpuint (flatpak_context_get_run_flags (context), ==,
                    (FLATPAK_RUN_FLAG_DEVEL |
                     FLATPAK_RUN_FLAG_MULTIARCH |
                     FLATPAK_RUN_FLAG_BLUETOOTH |
                     FLATPAK_RUN_FLAG_CANBUS));

  exports = flatpak_context_get_exports (context, "com.example.App");
  g_assert_nonnull (exports);

  g_clear_pointer (&exports, flatpak_exports_free);
  flatpak_context_append_bwrap_filesystem (context, bwrap,
                                           "com.example.App",
                                           NULL,
                                           NULL,
                                           &exports);
  print_bwrap (bwrap);
  g_assert_nonnull (exports);

  g_clear_pointer (&keyfile, g_key_file_unref);
  keyfile = g_key_file_new ();
  flatpak_context_save_metadata (context, FALSE, keyfile);
  text = g_key_file_to_data (keyfile, NULL, NULL);
  g_test_message ("Saved:\n%s", text);
  g_clear_pointer (&text, g_free);

  /* Test that keys round-trip back into the file */
  strv = g_key_file_get_string_list (keyfile, FLATPAK_METADATA_GROUP_CONTEXT,
                                     FLATPAK_METADATA_KEY_FILESYSTEMS,
                                     &n, &error);
  g_assert_nonnull (strv);
  /* The order is undefined, so sort them first */
  g_qsort_with_data (strv, n, sizeof (char *),
                     (GCompareDataFunc) flatpak_strcmp0_ptr, NULL);
  i = 0;
  g_assert_cmpstr (strv[i++], ==, "!/opt");
  g_assert_cmpstr (strv[i++], ==, "/home");
  g_assert_cmpstr (strv[i++], ==, "host");
  g_assert_cmpstr (strv[i], ==, NULL);
  g_assert_cmpuint (i, ==, n);
  g_clear_pointer (&strv, g_strfreev);

  strv = g_key_file_get_string_list (keyfile, FLATPAK_METADATA_GROUP_CONTEXT,
                                     FLATPAK_METADATA_KEY_SHARED,
                                     &n, &error);
  g_assert_no_error (error);
  g_assert_nonnull (strv);
  g_qsort_with_data (strv, n, sizeof (char *),
                     (GCompareDataFunc) flatpak_strcmp0_ptr, NULL);
  i = 0;
  g_assert_cmpstr (strv[i++], ==, "ipc");
  g_assert_cmpstr (strv[i++], ==, "network");
  g_assert_cmpstr (strv[i], ==, NULL);
  g_assert_cmpuint (i, ==, n);
  g_clear_pointer (&strv, g_strfreev);

  strv = g_key_file_get_string_list (keyfile, FLATPAK_METADATA_GROUP_CONTEXT,
                                     FLATPAK_METADATA_KEY_SOCKETS,
                                     &n, &error);
  g_assert_no_error (error);
  g_assert_nonnull (strv);
  g_qsort_with_data (strv, n, sizeof (char *),
                     (GCompareDataFunc) flatpak_strcmp0_ptr, NULL);
  i = 0;
  g_assert_cmpstr (strv[i++], ==, "cups");
  g_assert_cmpstr (strv[i++], ==, "fallback-x11");
  g_assert_cmpstr (strv[i++], ==, "pcsc");
  g_assert_cmpstr (strv[i++], ==, "pulseaudio");
  g_assert_cmpstr (strv[i++], ==, "session-bus");
  g_assert_cmpstr (strv[i++], ==, "ssh-auth");
  g_assert_cmpstr (strv[i++], ==, "system-bus");
  g_assert_cmpstr (strv[i++], ==, "wayland");
  g_assert_cmpstr (strv[i++], ==, "x11");
  g_assert_cmpstr (strv[i], ==, NULL);
  g_assert_cmpuint (i, ==, n);
  g_clear_pointer (&strv, g_strfreev);

  strv = g_key_file_get_string_list (keyfile, FLATPAK_METADATA_GROUP_CONTEXT,
                                     FLATPAK_METADATA_KEY_DEVICES,
                                     &n, &error);
  g_assert_no_error (error);
  g_assert_nonnull (strv);
  g_qsort_with_data (strv, n, sizeof (char *),
                     (GCompareDataFunc) flatpak_strcmp0_ptr, NULL);
  i = 0;
  g_assert_cmpstr (strv[i++], ==, "all");
  g_assert_cmpstr (strv[i++], ==, "dri");
  g_assert_cmpstr (strv[i++], ==, "kvm");
  g_assert_cmpstr (strv[i++], ==, "shm");
  g_assert_cmpstr (strv[i], ==, NULL);
  g_assert_cmpuint (i, ==, n);
  g_clear_pointer (&strv, g_strfreev);

  strv = g_key_file_get_string_list (keyfile, FLATPAK_METADATA_GROUP_CONTEXT,
                                     FLATPAK_METADATA_KEY_PERSISTENT,
                                     &n, &error);
  g_assert_no_error (error);
  g_assert_nonnull (strv);
  g_qsort_with_data (strv, n, sizeof (char *),
                     (GCompareDataFunc) flatpak_strcmp0_ptr, NULL);
  i = 0;
  g_assert_cmpstr (strv[i++], ==, ".openarena");
  g_assert_cmpstr (strv[i], ==, NULL);
  g_assert_cmpuint (i, ==, n);
  g_clear_pointer (&strv, g_strfreev);

  strv = g_key_file_get_keys (keyfile, FLATPAK_METADATA_GROUP_SESSION_BUS_POLICY,
                              &n, &error);
  g_assert_no_error (error);
  g_assert_nonnull (strv);
  g_qsort_with_data (strv, n, sizeof (char *),
                     (GCompareDataFunc) flatpak_strcmp0_ptr, NULL);
  i = 0;
  g_assert_cmpstr (strv[i++], ==, "org.example.SessionService");
  g_assert_cmpstr (strv[i], ==, NULL);
  g_assert_cmpuint (i, ==, n);
  g_clear_pointer (&strv, g_strfreev);

  text = g_key_file_get_string (keyfile, FLATPAK_METADATA_GROUP_SESSION_BUS_POLICY,
                                "org.example.SessionService", &error);
  g_assert_no_error (error);
  g_assert_cmpstr (text, ==, "own");
  g_clear_pointer (&text, g_free);

  strv = g_key_file_get_keys (keyfile, FLATPAK_METADATA_GROUP_SYSTEM_BUS_POLICY,
                              &n, &error);
  g_assert_no_error (error);
  g_assert_nonnull (strv);
  g_qsort_with_data (strv, n, sizeof (char *),
                     (GCompareDataFunc) flatpak_strcmp0_ptr, NULL);
  i = 0;
  g_assert_cmpstr (strv[i++], ==, "net.example.SystemService");
  g_assert_cmpstr (strv[i], ==, NULL);
  g_assert_cmpuint (i, ==, n);
  g_clear_pointer (&strv, g_strfreev);

  text = g_key_file_get_string (keyfile, FLATPAK_METADATA_GROUP_SYSTEM_BUS_POLICY,
                                "net.example.SystemService", &error);
  g_assert_no_error (error);
  g_assert_cmpstr (text, ==, "talk");
  g_clear_pointer (&text, g_free);

  strv = g_key_file_get_keys (keyfile, FLATPAK_METADATA_GROUP_ENVIRONMENT,
                              &n, &error);
  g_assert_no_error (error);
  g_assert_nonnull (strv);
  g_qsort_with_data (strv, n, sizeof (char *),
                     (GCompareDataFunc) flatpak_strcmp0_ptr, NULL);
  i = 0;
  g_assert_cmpstr (strv[i++], ==, "HYPOTHETICAL_PATH");
  g_assert_cmpstr (strv[i], ==, NULL);
  g_assert_cmpuint (i, ==, n);
  g_clear_pointer (&strv, g_strfreev);

  text = g_key_file_get_string (keyfile, FLATPAK_METADATA_GROUP_ENVIRONMENT,
                                "HYPOTHETICAL_PATH", &error);
  g_assert_no_error (error);
  g_assert_cmpstr (text, ==, "/foo:/bar");
  g_clear_pointer (&text, g_free);

  strv = g_key_file_get_keys (keyfile, FLATPAK_METADATA_GROUP_PREFIX_POLICY "MyPolicy",
                              &n, &error);
  g_assert_no_error (error);
  g_assert_nonnull (strv);
  g_qsort_with_data (strv, n, sizeof (char *),
                     (GCompareDataFunc) flatpak_strcmp0_ptr, NULL);
  i = 0;
  g_assert_cmpstr (strv[i++], ==, "Colours");
  g_assert_cmpstr (strv[i], ==, NULL);
  g_assert_cmpuint (i, ==, n);
  g_clear_pointer (&strv, g_strfreev);

  strv = g_key_file_get_string_list (keyfile, FLATPAK_METADATA_GROUP_PREFIX_POLICY "MyPolicy",
                                     "Colours", &n, &error);
  g_assert_no_error (error);
  g_assert_nonnull (strv);
  g_qsort_with_data (strv, n, sizeof (char *),
                     (GCompareDataFunc) flatpak_strcmp0_ptr, NULL);
  i = 0;
  g_assert_cmpstr (strv[i++], ==, "blue");
  g_assert_cmpstr (strv[i++], ==, "green");
  g_assert_cmpstr (strv[i], ==, NULL);
  g_assert_cmpuint (i, ==, n);
  g_clear_pointer (&strv, g_strfreev);
}