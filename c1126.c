goa_http_client_check (GoaHttpClient       *client,
                       const gchar         *uri,
                       const gchar         *username,
                       const gchar         *password,
                       gboolean             accept_ssl_errors,
                       GCancellable        *cancellable,
                       GAsyncReadyCallback  callback,
                       gpointer             user_data)
{
  CheckData *data;
  CheckAuthData *auth;

  g_return_if_fail (GOA_IS_HTTP_CLIENT (client));
  g_return_if_fail (uri != NULL || uri[0] != '\0');
  g_return_if_fail (username != NULL || username[0] != '\0');
  g_return_if_fail (password != NULL || password[0] != '\0');
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

  data = g_slice_new0 (CheckData);
  data->res = g_simple_async_result_new (G_OBJECT (client), callback, user_data, goa_http_client_check);
  data->session = soup_session_async_new_with_options (SOUP_SESSION_SSL_USE_SYSTEM_CA_FILE, TRUE,
                                                       SOUP_SESSION_SSL_STRICT, FALSE,
                                                       SOUP_SESSION_USE_THREAD_CONTEXT, TRUE,
                                                       NULL);

  data->accept_ssl_errors = accept_ssl_errors;

  data->msg = soup_message_new (SOUP_METHOD_GET, uri);
  soup_message_headers_append (data->msg->request_headers, "Connection", "close");

  if (cancellable != NULL)
    {
      data->cancellable = g_object_ref (cancellable);
      data->cancellable_id = g_cancellable_connect (data->cancellable,
                                                    G_CALLBACK (http_client_check_cancelled_cb),
                                                    data,
                                                    NULL);
      g_simple_async_result_set_check_cancellable (data->res, data->cancellable);
    }

  auth = g_slice_new0 (CheckAuthData);
  auth->username = g_strdup (username);
  auth->password = g_strdup (password);
  g_signal_connect_data (data->session,
                         "authenticate",
                         G_CALLBACK (http_client_authenticate),
                         auth,
                         http_client_check_auth_data_free,
                         0);

  soup_session_queue_message (data->session, data->msg, http_client_check_response_cb, data);
}