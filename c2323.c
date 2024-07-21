HttpTransact::build_error_response(State *s, HTTPStatus status_code, const char *reason_phrase_or_null,
                                   const char *error_body_type, const char *format, ...)
{
  va_list ap;
  const char *reason_phrase;
  char *url_string;
  char body_language[256], body_type[256];

  if (NULL == error_body_type) {
    error_body_type = "default";
  }

  ////////////////////////////////////////////////////////////
  // get the url --- remember this is dynamically allocated //
  ////////////////////////////////////////////////////////////
  if (s->hdr_info.client_request.valid()) {
    url_string = s->hdr_info.client_request.url_string_get(&s->arena);
  } else {
    url_string = NULL;
  }

  // Make sure that if this error occured before we initailzied the state variables that we do now.
  initialize_state_variables_from_request(s, &s->hdr_info.client_request);

  //////////////////////////////////////////////////////
  //  If there is a request body, we must disable     //
  //  keep-alive to prevent the body being read as    //
  //  the next header (unless we've already drained   //
  //  which we do for NTLM auth)                      //
  //////////////////////////////////////////////////////
  if (status_code == HTTP_STATUS_REQUEST_TIMEOUT ||
      s->hdr_info.client_request.get_content_length() != 0 ||
      s->client_info.transfer_encoding == HttpTransact::CHUNKED_ENCODING) {
    s->client_info.keep_alive = HTTP_NO_KEEPALIVE;
  } else {
    // We don't have a request body.  Since we are
    //  generating the error, we know we can trust
    //  the content-length
    s->hdr_info.trust_response_cl = true;
  }

  switch (status_code) {
  case HTTP_STATUS_BAD_REQUEST:
    SET_VIA_STRING(VIA_CLIENT_REQUEST, VIA_CLIENT_ERROR);
    SET_VIA_STRING(VIA_ERROR_TYPE, VIA_ERROR_HEADER_SYNTAX);
    break;
  case HTTP_STATUS_BAD_GATEWAY:
    SET_VIA_STRING(VIA_ERROR_TYPE, VIA_ERROR_CONNECTION);
    break;
  case HTTP_STATUS_GATEWAY_TIMEOUT:
    SET_VIA_STRING(VIA_ERROR_TYPE, VIA_ERROR_TIMEOUT);
    break;
  case HTTP_STATUS_NOT_FOUND:
    SET_VIA_STRING(VIA_ERROR_TYPE, VIA_ERROR_SERVER);
    break;
  case HTTP_STATUS_FORBIDDEN:
    SET_VIA_STRING(VIA_CLIENT_REQUEST, VIA_CLIENT_ERROR);
    SET_VIA_STRING(VIA_ERROR_TYPE, VIA_ERROR_FORBIDDEN);
    break;
  case HTTP_STATUS_HTTPVER_NOT_SUPPORTED:
    SET_VIA_STRING(VIA_CLIENT_REQUEST, VIA_CLIENT_ERROR);
    SET_VIA_STRING(VIA_ERROR_TYPE, VIA_ERROR_SERVER);
    break;
  case HTTP_STATUS_INTERNAL_SERVER_ERROR:
    SET_VIA_STRING(VIA_ERROR_TYPE, VIA_ERROR_DNS_FAILURE);
    break;
  case HTTP_STATUS_MOVED_TEMPORARILY:
    SET_VIA_STRING(VIA_ERROR_TYPE, VIA_ERROR_SERVER);
    break;
  case HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED:
    SET_VIA_STRING(VIA_CLIENT_REQUEST, VIA_CLIENT_ERROR);
    SET_VIA_STRING(VIA_ERROR_TYPE, VIA_ERROR_AUTHORIZATION);
    break;
  case HTTP_STATUS_UNAUTHORIZED:
    SET_VIA_STRING(VIA_CLIENT_REQUEST, VIA_CLIENT_ERROR);
    SET_VIA_STRING(VIA_ERROR_TYPE, VIA_ERROR_AUTHORIZATION);
    break;
  default:
    break;
  }

  reason_phrase = (reason_phrase_or_null ? reason_phrase_or_null : (char *) (http_hdr_reason_lookup(status_code)));
  if (unlikely(!reason_phrase))
    reason_phrase = "Unknown HTTP Status";

  // set the source to internal so that chunking is handled correctly
  s->source = SOURCE_INTERNAL;
  build_response(s, &s->hdr_info.client_response, s->client_info.http_version, status_code, reason_phrase);

  if (status_code == HTTP_STATUS_SERVICE_UNAVAILABLE) {
    if (s->pCongestionEntry != NULL) {
      int ret_tmp;
      int retry_after = s->pCongestionEntry->client_retry_after();

      s->congestion_control_crat = retry_after;
      if (s->hdr_info.client_response.value_get(MIME_FIELD_RETRY_AFTER, MIME_LEN_RETRY_AFTER, &ret_tmp) == NULL)
        s->hdr_info.client_response.value_set_int(MIME_FIELD_RETRY_AFTER, MIME_LEN_RETRY_AFTER, retry_after);
    }
  }
  if (status_code == HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED &&
      s->method == HTTP_WKSIDX_CONNECT && s->hdr_info.client_response.presence(MIME_PRESENCE_PROXY_CONNECTION)) {
    int has_ua_msie = 0;
    int user_agent_value_len, slen;
    const char *user_agent_value, *c, *e;

    user_agent_value = s->hdr_info.client_request.value_get(MIME_FIELD_USER_AGENT, MIME_LEN_USER_AGENT, &user_agent_value_len);
    if (user_agent_value && user_agent_value_len >= 4) {
      c = user_agent_value;
      e = c + user_agent_value_len - 4;
      while (1) {
        slen = (int) (e - c);
        c = (const char *) memchr(c, 'M', slen);
        if (c == NULL || (e - c) < 3)
          break;
        if ((c[1] == 'S') && (c[2] == 'I') && (c[3] == 'E')) {
          has_ua_msie = 1;
          break;
        }
        c++;
      }
    }

    if (has_ua_msie)
      s->hdr_info.client_response.value_set(MIME_FIELD_PROXY_CONNECTION, MIME_LEN_PROXY_CONNECTION, "close", 5);
  }
  // Add a bunch of headers to make sure that caches between
  // the Traffic Server and the client do not cache the error
  // page.
  s->hdr_info.client_response.value_set(MIME_FIELD_CACHE_CONTROL, MIME_LEN_CACHE_CONTROL, "no-store", 8);
  // Make sure there are no Expires and Last-Modified headers.
  s->hdr_info.client_response.field_delete(MIME_FIELD_EXPIRES, MIME_LEN_EXPIRES);
  s->hdr_info.client_response.field_delete(MIME_FIELD_LAST_MODIFIED, MIME_LEN_LAST_MODIFIED);

  if ((status_code == HTTP_STATUS_TEMPORARY_REDIRECT ||
       status_code == HTTP_STATUS_MOVED_TEMPORARILY ||
       status_code == HTTP_STATUS_MOVED_PERMANENTLY) &&
      s->remap_redirect) {
    s->hdr_info.client_response.value_set(MIME_FIELD_LOCATION, MIME_LEN_LOCATION, s->remap_redirect, strlen(s->remap_redirect));
  }



  ////////////////////////////////////////////////////////////////////
  // create the error message using the "body factory", which will  //
  // build a customized error message if available, or generate the //
  // old style internal defaults otherwise --- the body factory     //
  // supports language targeting using the Accept-Language header   //
  ////////////////////////////////////////////////////////////////////

  int64_t len;
  char * new_msg;

  va_start(ap, format);
  new_msg = body_factory->fabricate_with_old_api(error_body_type, s, 8192,
                                                       &len,
                                                       body_language, sizeof(body_language),
                                                       body_type, sizeof(body_type),
                                                       format, ap);
  va_end(ap);

  // After the body factory is called, a new "body" is allocated, and we must replace it. It is
  // unfortunate that there's no way to avoid this fabrication even when there is no substitutions...
  s->free_internal_msg_buffer();
  s->internal_msg_buffer = new_msg;
  s->internal_msg_buffer_size = len;
  s->internal_msg_buffer_index = 0;
  s->internal_msg_buffer_fast_allocator_size = -1;

  s->hdr_info.client_response.value_set(MIME_FIELD_CONTENT_TYPE, MIME_LEN_CONTENT_TYPE, body_type, strlen(body_type));
  s->hdr_info.client_response.value_set(MIME_FIELD_CONTENT_LANGUAGE, MIME_LEN_CONTENT_LANGUAGE, body_language,
                                        strlen(body_language));

  ////////////////////////////////////////
  // log a description in the error log //
  ////////////////////////////////////////

  if (s->current.state == CONNECTION_ERROR) {
    char *reason_buffer;
    int buf_len = sizeof(char) * (strlen(get_error_string(s->cause_of_death_errno)) + 50);
    reason_buffer = (char *) alloca(buf_len);
    snprintf(reason_buffer, buf_len, "Connect Error <%s/%d>", get_error_string(s->cause_of_death_errno), s->cause_of_death_errno);
    reason_phrase = reason_buffer;
  }

  if (s->http_config_param->errors_log_error_pages && status_code >= HTTP_STATUS_BAD_REQUEST) {
    char ip_string[INET6_ADDRSTRLEN];

    Log::error("RESPONSE: sent %s status %d (%s) for '%s'",
        ats_ip_ntop(&s->client_info.addr.sa, ip_string, sizeof(ip_string)),
        status_code,
        reason_phrase,
        (url_string ? url_string : "<none>"));
  }

  if (url_string) {
    s->arena.str_free(url_string);
  }

  s->next_action = SM_ACTION_SEND_ERROR_CACHE_NOOP;
  return;
}