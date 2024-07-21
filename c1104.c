http_client_check_response_cb (SoupSession *session, SoupMessage *msg, gpointer user_data)
{
  GError *error;
  CheckData *data = user_data;
  gboolean op_res;

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

  op_res = TRUE;

 out:
  g_simple_async_result_set_op_res_gboolean (data->res, op_res);
  if (error != NULL)
    g_simple_async_result_take_error (data->res, error);

  g_simple_async_result_complete_in_idle (data->res);
  g_idle_add (http_client_check_data_free, data);
}