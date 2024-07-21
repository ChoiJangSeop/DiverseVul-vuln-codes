ews_client_autodiscover_response_cb (SoupSession *session, SoupMessage *msg, gpointer user_data)
{
  GError *error;
  AutodiscoverData *data = user_data;
  gboolean op_res;
  guint status;
  gint idx;
  gsize size;
  xmlDoc *doc;
  xmlNode *node;

  status = msg->status_code;
  if (status == SOUP_STATUS_CANCELLED)
    return;

  error = NULL;
  op_res = FALSE;
  size = sizeof (data->msgs) / sizeof (data->msgs[0]);

  for (idx = 0; idx < size; idx++)
    {
      if (data->msgs[idx] == msg)
        break;
    }
  if (idx == size)
    return;

  data->msgs[idx] = NULL;

  if (status != SOUP_STATUS_OK)
    {
      g_set_error (&error,
                   GOA_ERROR,
                   GOA_ERROR_FAILED, /* TODO: more specific */
                   _("Code: %u - Unexpected response from server"),
                   status);
      goto out;
    }

  soup_buffer_free (soup_message_body_flatten (SOUP_MESSAGE (msg)->response_body));
  g_debug ("The response headers");
  g_debug ("===================");
  g_debug ("%s", SOUP_MESSAGE (msg)->response_body->data);

  doc = xmlReadMemory (msg->response_body->data, msg->response_body->length, "autodiscover.xml", NULL, 0);
  if (doc == NULL)
    {
      g_set_error (&error,
                   GOA_ERROR,
                   GOA_ERROR_FAILED, /* TODO: more specific */
                   _("Failed to parse autodiscover response XML"));
      goto out;
    }

  node = xmlDocGetRootElement (doc);
  if (g_strcmp0 ((gchar *) node->name, "Autodiscover"))
    {
      g_set_error (&error,
                   GOA_ERROR,
                   GOA_ERROR_FAILED, /* TODO: more specific */
                   _("Failed to find Autodiscover element"));
      goto out;
    }

  for (node = node->children; node; node = node->next)
    {
      if (ews_client_check_node (node, "Response"))
        break;
    }
  if (node == NULL)
    {
      g_set_error (&error,
                   GOA_ERROR,
                   GOA_ERROR_FAILED, /* TODO: more specific */
                   _("Failed to find Response element"));
      goto out;
    }

  for (node = node->children; node; node = node->next)
    {
      if (ews_client_check_node (node, "Account"))
        break;
    }
  if (node == NULL)
    {
      g_set_error (&error,
                   GOA_ERROR,
                   GOA_ERROR_FAILED, /* TODO: more specific */
                   _("Failed to find Account element"));
      goto out;
    }

  for (node = node->children; node; node = node->next)
    {
      if (ews_client_check_node (node, "Protocol"))
        {
          op_res = ews_client_autodiscover_parse_protocol (node);
          break;
        }
    }
  if (!op_res)
    {
      g_set_error (&error,
                   GOA_ERROR,
                   GOA_ERROR_FAILED, /* TODO: more specific*/
                   _("Failed to find ASUrl and OABUrl in autodiscover response"));
      goto out;
    }

  for (idx = 0; idx < size; idx++)
    {
      if (data->msgs[idx] != NULL)
        {
          /* Since we are cancelling from the same thread that we queued the
           * message, the callback (ie. this function) will be invoked before
           * soup_session_cancel_message returns.
           */
          soup_session_cancel_message (data->session, data->msgs[idx], SOUP_STATUS_CANCELLED);
          data->msgs[idx] = NULL;
        }
    }

 out:
  if (error != NULL)
    {
      for (idx = 0; idx < size; idx++)
        {
          if (data->msgs[idx] != NULL)
            {
              /* There's another request outstanding.
               * Hope that it has better luck.
               */
              g_clear_error (&error);
              return;
            }
        }
      g_simple_async_result_set_from_error (data->res, error);
    }
  else
    g_simple_async_result_set_op_res_gboolean (data->res, op_res);

  g_simple_async_result_complete_in_idle (data->res);
  ews_client_autodiscover_data_free (data);
}