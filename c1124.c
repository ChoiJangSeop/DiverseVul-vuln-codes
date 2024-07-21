http_client_check_response_cb (SoupSession *session, SoupMessage *msg, gpointer user_data)
{
  GError *error;
  CheckData *data = user_data;
  GTlsCertificateFlags cert_flags;
  gboolean op_res;
  gboolean using_https;

  error = NULL;
  op_res = FALSE;

  if (msg->status_code == SOUP_STATUS_CANCELLED)
    goto out;
  else if (msg->status_code != SOUP_STATUS_OK)
    {
      g_set_error (&error,
                   GOA_ERROR,
                   GOA_ERROR_FAILED, /* TODO: more specific */
                   _("Code: %u - Unexpected response from server"),
                   msg->status_code);
      goto out;
    }

  if (!data->accept_ssl_errors)
    {
      using_https = soup_message_get_https_status (msg, NULL, &cert_flags);
      if (using_https && cert_flags != 0)
        {
          goa_utils_set_error_ssl (&error, cert_flags);
          goto out;
        }
    }

  op_res = TRUE;

 out:
  g_simple_async_result_set_op_res_gboolean (data->res, op_res);
  if (error != NULL)
    g_simple_async_result_take_error (data->res, error);

  g_simple_async_result_complete_in_idle (data->res);
  g_idle_add (http_client_check_data_free, data);
}