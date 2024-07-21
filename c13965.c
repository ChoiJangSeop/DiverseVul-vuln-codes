append_header (GString *string,
               const gchar *name,
               const gchar *value)
{
  if (value)
    {
      g_return_val_if_fail (cockpit_web_response_is_simple_token (name), 0);
      g_return_val_if_fail (cockpit_web_response_is_header_value (value), 0);
      g_string_append_printf (string, "%s: %s\r\n", name, value);
    }
  if (g_ascii_strcasecmp ("Content-Type", name) == 0)
    return HEADER_CONTENT_TYPE;
  if (g_ascii_strcasecmp ("Cache-Control", name) == 0)
    return HEADER_CACHE_CONTROL;
  if (g_ascii_strcasecmp ("Vary", name) == 0)
    return HEADER_VARY;
  if (g_ascii_strcasecmp ("Content-Encoding", name) == 0)
    return HEADER_CONTENT_ENCODING;
  if (g_ascii_strcasecmp ("X-DNS-Prefetch-Control", name) == 0)
    return HEADER_DNS_PREFETCH_CONTROL;
  if (g_ascii_strcasecmp ("Referrer-Policy", name) == 0)
    return HEADER_REFERRER_POLICY;
  if (g_ascii_strcasecmp ("X-Content-Type-Options", name) == 0)
    return HEADER_CONTENT_TYPE_OPTIONS;
  if (g_ascii_strcasecmp ("Cross-Origin-Resource-Policy", name) == 0)
    return HEADER_CROSS_ORIGIN_RESOURCE_POLICY;
  if (g_ascii_strcasecmp ("Content-Length", name) == 0 ||
      g_ascii_strcasecmp ("Transfer-Encoding", name) == 0 ||
      g_ascii_strcasecmp ("Connection", name) == 0)
    {
      g_critical ("Don't set %s header manually. This is a programmer error.", name);
    }
  return 0;
}