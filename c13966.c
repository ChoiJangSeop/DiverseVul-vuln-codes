finish_headers (CockpitWebResponse *self,
                GString *string,
                gssize length,
                gint status,
                guint seen)
{
  const gchar *content_type;

  /* Automatically figure out content type */
  if ((seen & HEADER_CONTENT_TYPE) == 0 &&
      self->full_path != NULL && status >= 200 && status <= 299)
    {
      content_type = cockpit_web_response_content_type (self->full_path);
      if (content_type)
        g_string_append_printf (string, "Content-Type: %s\r\n", content_type);
    }

  if (status != 304)
    {
      if (length < 0 || seen & HEADER_CONTENT_ENCODING || self->filters)
        {
          self->chunked = TRUE;
          g_string_append_printf (string, "Transfer-Encoding: chunked\r\n");
        }
      else
        {
          self->chunked = FALSE;
          g_string_append_printf (string, "Content-Length: %" G_GSSIZE_FORMAT "\r\n", length);
          self->out_queueable = length;
        }
    }

  if ((seen & HEADER_CACHE_CONTROL) == 0 && status >= 200 && status <= 299)
    {
      if (self->cache_type == COCKPIT_WEB_RESPONSE_CACHE_FOREVER)
        g_string_append (string, "Cache-Control: max-age=31556926, public\r\n");
      else if (self->cache_type == COCKPIT_WEB_RESPONSE_NO_CACHE)
        g_string_append (string, "Cache-Control: no-cache, no-store\r\n");
      else if (self->cache_type == COCKPIT_WEB_RESPONSE_CACHE_PRIVATE)
        g_string_append (string, "Cache-Control: max-age=86400, private\r\n");
    }

  if ((seen & HEADER_VARY) == 0 && status >= 200 && status <= 299 &&
      self->cache_type == COCKPIT_WEB_RESPONSE_CACHE_PRIVATE)
    {
      g_string_append (string, "Vary: Cookie\r\n");
    }

  if (!self->keep_alive)
    g_string_append (string, "Connection: close\r\n");

  /* Some blanket security headers */
  if ((seen & HEADER_DNS_PREFETCH_CONTROL) == 0)
    g_string_append (string, "X-DNS-Prefetch-Control: off\r\n");
  if ((seen & HEADER_REFERRER_POLICY) == 0)
    g_string_append (string, "Referrer-Policy: no-referrer\r\n");
  if ((seen & HEADER_CONTENT_TYPE_OPTIONS) == 0)
    g_string_append (string, "X-Content-Type-Options: nosniff\r\n");
  /* Be very strict here -- there is no reason that external web sites should
   * be able to read any resource. This does *not* affect embedding with <iframe> */
  if ((seen & HEADER_CROSS_ORIGIN_RESOURCE_POLICY) == 0)
    g_string_append (string, "Cross-Origin-Resource-Policy: same-origin\r\n");

  g_string_append (string, "\r\n");
  return g_string_free_to_bytes (string);
}