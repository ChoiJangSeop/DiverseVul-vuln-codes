event_raise(uschar * action, const uschar * event, uschar * ev_data)
{
uschar * s;
if (action)
  {
  DEBUG(D_deliver)
    debug_printf("Event(%s): event_action=|%s| delivery_IP=%s\n",
      event,
      action, deliver_host_address);

  event_name = event;
  event_data = ev_data;

  if (!(s = expand_string(action)) && *expand_string_message)
    log_write(0, LOG_MAIN|LOG_PANIC,
      "failed to expand event_action %s in %s: %s\n",
      event, transport_name, expand_string_message);

  event_name = event_data = NULL;

  /* If the expansion returns anything but an empty string, flag for
  the caller to modify his normal processing
  */
  if (s && *s)
    {
    DEBUG(D_deliver)
      debug_printf("Event(%s): event_action returned \"%s\"\n", event, s);
    return s;
    }
  }
return NULL;
}