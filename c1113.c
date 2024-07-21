goa_web_view_init (GoaWebView *self)
{
  GoaWebViewPrivate *priv;
  GtkWidget *scrolled_window;
  SoupCookieJar *cookie_jar;
  SoupLogger *logger;
  SoupSession *session;
  WebKitWebSettings *settings;

  self->priv = GOA_WEB_VIEW_GET_PRIVATE (self);
  priv = self->priv;

  session = webkit_get_default_session ();

  soup_session_add_feature_by_type (session, SOUP_TYPE_PROXY_RESOLVER_DEFAULT);
  g_object_set (session, "accept-language-auto", TRUE, NULL);

  soup_session_remove_feature_by_type (session, SOUP_TYPE_COOKIE_JAR);
  cookie_jar = soup_cookie_jar_new ();
  soup_session_add_feature (session, SOUP_SESSION_FEATURE (cookie_jar));
  g_object_unref (cookie_jar);

  logger = soup_logger_new (SOUP_LOGGER_LOG_BODY, -1);
  soup_logger_set_printer (logger, web_view_log_printer, NULL, NULL);
  soup_session_add_feature (session, SOUP_SESSION_FEATURE (logger));
  g_object_unref (logger);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scrolled_window, 500, 400);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (self), scrolled_window);

  priv->web_view = webkit_web_view_new ();
  priv->status = WEBKIT_LOAD_PROVISIONAL;
  gtk_container_add (GTK_CONTAINER (scrolled_window), priv->web_view);

  settings = webkit_web_view_get_settings (WEBKIT_WEB_VIEW (priv->web_view));
  g_object_set (settings, "user-stylesheet-uri", "file://" PACKAGE_DATA_DIR "/goawebview.css", NULL);

  /* statusbar is hidden by default */
  priv->floating_bar = nautilus_floating_bar_new (NULL, FALSE);
  gtk_widget_set_halign (priv->floating_bar, GTK_ALIGN_START);
  gtk_widget_set_valign (priv->floating_bar, GTK_ALIGN_END);
  gtk_widget_set_no_show_all (priv->floating_bar, TRUE);
  gtk_overlay_add_overlay (GTK_OVERLAY (self), priv->floating_bar);

  priv->progress_bar = gtk_progress_bar_new ();
  gtk_style_context_add_class (gtk_widget_get_style_context (priv->progress_bar),
                               GTK_STYLE_CLASS_OSD);
  gtk_widget_set_halign (priv->progress_bar, GTK_ALIGN_FILL);
  gtk_widget_set_valign (priv->progress_bar, GTK_ALIGN_START);
  gtk_overlay_add_overlay (GTK_OVERLAY (self), priv->progress_bar);

  priv->notify_progress_id = g_signal_connect (priv->web_view,
                                               "notify::progress",
                                               G_CALLBACK (web_view_notify_progress_cb),
                                               self);
  priv->notify_load_status_id = g_signal_connect (priv->web_view,
                                                  "notify::load-status",
                                                  G_CALLBACK (web_view_notify_load_status_cb),
                                                  self);
}