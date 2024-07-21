HttpTransact::build_redirect_response(State* s)
{
  DebugTxn("http_redirect", "[HttpTransact::build_redirect_response]");
  URL *u;
  const char *old_host;
  int old_host_len;
  const char *new_url = NULL;
  int new_url_len;
  char *to_free = NULL;
  char body_language[256], body_type[256];

  HTTPStatus status_code = HTTP_STATUS_MOVED_TEMPORARILY;
  char *reason_phrase = (char *) (http_hdr_reason_lookup(status_code));

  build_response(s, &s->hdr_info.client_response, s->client_info.http_version, status_code, reason_phrase);

  //////////////////////////////////////////////////////////
  // figure out what new url should be.  this little hack //
  // inserts expanded hostname into old url in order to   //
  // get scheme information, then puts the old url back.  //
  //////////////////////////////////////////////////////////
  u = s->hdr_info.client_request.url_get();
  old_host = u->host_get(&old_host_len);
  u->host_set(s->dns_info.lookup_name, strlen(s->dns_info.lookup_name));
  new_url = to_free = u->string_get(&s->arena, &new_url_len);
  if (new_url == NULL) {
    new_url = "";
  }
  u->host_set(old_host, old_host_len);

  //////////////////////////
  // set redirect headers //
  //////////////////////////
  HTTPHdr *h = &s->hdr_info.client_response;
  if (s->txn_conf->insert_response_via_string) {
    const char pa[] = "Proxy-agent";

    h->value_append(pa, sizeof(pa) - 1, s->http_config_param->proxy_response_via_string,
                    s->http_config_param->proxy_response_via_string_len);
  }
  h->value_set(MIME_FIELD_LOCATION, MIME_LEN_LOCATION, new_url, new_url_len);

  //////////////////////////
  // set descriptive text //
  //////////////////////////
  s->internal_msg_buffer_index = 0;
  s->free_internal_msg_buffer();
  s->internal_msg_buffer_fast_allocator_size = -1;
  s->internal_msg_buffer = body_factory->fabricate_with_old_api_build_va("redirect#moved_temporarily", s, 8192,
                                                                         &s->internal_msg_buffer_size,
                                                                         body_language, sizeof(body_language),
                                                                         body_type, sizeof(body_type),
                                                                         "%s <a href=\"%s\">%s</a>.  %s.",
                                                                         "The document you requested is now",
                                                                         new_url, new_url,
                                                                         "Please update your documents and bookmarks accordingly", NULL);


  h->set_content_length(s->internal_msg_buffer_size);
  h->value_set(MIME_FIELD_CONTENT_TYPE, MIME_LEN_CONTENT_TYPE, "text/html", 9);

  s->arena.str_free(to_free);
}