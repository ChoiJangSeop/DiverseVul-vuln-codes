CURLcode Curl_add_custom_headers(struct Curl_easy *data,
                                 bool is_connect,
#ifndef USE_HYPER
                                 struct dynbuf *req
#else
                                 void *req
#endif
  )
{
  struct connectdata *conn = data->conn;
  char *ptr;
  struct curl_slist *h[2];
  struct curl_slist *headers;
  int numlists = 1; /* by default */
  int i;

#ifndef CURL_DISABLE_PROXY
  enum proxy_use proxy;

  if(is_connect)
    proxy = HEADER_CONNECT;
  else
    proxy = conn->bits.httpproxy && !conn->bits.tunnel_proxy?
      HEADER_PROXY:HEADER_SERVER;

  switch(proxy) {
  case HEADER_SERVER:
    h[0] = data->set.headers;
    break;
  case HEADER_PROXY:
    h[0] = data->set.headers;
    if(data->set.sep_headers) {
      h[1] = data->set.proxyheaders;
      numlists++;
    }
    break;
  case HEADER_CONNECT:
    if(data->set.sep_headers)
      h[0] = data->set.proxyheaders;
    else
      h[0] = data->set.headers;
    break;
  }
#else
  (void)is_connect;
  h[0] = data->set.headers;
#endif

  /* loop through one or two lists */
  for(i = 0; i < numlists; i++) {
    headers = h[i];

    while(headers) {
      char *semicolonp = NULL;
      ptr = strchr(headers->data, ':');
      if(!ptr) {
        char *optr;
        /* no colon, semicolon? */
        ptr = strchr(headers->data, ';');
        if(ptr) {
          optr = ptr;
          ptr++; /* pass the semicolon */
          while(*ptr && ISSPACE(*ptr))
            ptr++;

          if(*ptr) {
            /* this may be used for something else in the future */
            optr = NULL;
          }
          else {
            if(*(--ptr) == ';') {
              /* copy the source */
              semicolonp = strdup(headers->data);
              if(!semicolonp) {
#ifndef USE_HYPER
                Curl_dyn_free(req);
#endif
                return CURLE_OUT_OF_MEMORY;
              }
              /* put a colon where the semicolon is */
              semicolonp[ptr - headers->data] = ':';
              /* point at the colon */
              optr = &semicolonp [ptr - headers->data];
            }
          }
          ptr = optr;
        }
      }
      if(ptr && (ptr != headers->data)) {
        /* we require a colon for this to be a true header */

        ptr++; /* pass the colon */
        while(*ptr && ISSPACE(*ptr))
          ptr++;

        if(*ptr || semicolonp) {
          /* only send this if the contents was non-blank or done special */
          CURLcode result = CURLE_OK;
          char *compare = semicolonp ? semicolonp : headers->data;

          if(data->state.aptr.host &&
             /* a Host: header was sent already, don't pass on any custom Host:
                header as that will produce *two* in the same request! */
             checkprefix("Host:", compare))
            ;
          else if(data->state.httpreq == HTTPREQ_POST_FORM &&
                  /* this header (extended by formdata.c) is sent later */
                  checkprefix("Content-Type:", compare))
            ;
          else if(data->state.httpreq == HTTPREQ_POST_MIME &&
                  /* this header is sent later */
                  checkprefix("Content-Type:", compare))
            ;
          else if(conn->bits.authneg &&
                  /* while doing auth neg, don't allow the custom length since
                     we will force length zero then */
                  checkprefix("Content-Length:", compare))
            ;
          else if(data->state.aptr.te &&
                  /* when asking for Transfer-Encoding, don't pass on a custom
                     Connection: */
                  checkprefix("Connection:", compare))
            ;
          else if((conn->httpversion >= 20) &&
                  checkprefix("Transfer-Encoding:", compare))
            /* HTTP/2 doesn't support chunked requests */
            ;
          else if((checkprefix("Authorization:", compare) ||
                   checkprefix("Cookie:", compare)) &&
                  /* be careful of sending this potentially sensitive header to
                     other hosts */
                  !allow_auth_to_host(data))
            ;
          else {
#ifdef USE_HYPER
            result = Curl_hyper_header(data, req, compare);
#else
            result = Curl_dyn_addf(req, "%s\r\n", compare);
#endif
          }
          if(semicolonp)
            free(semicolonp);
          if(result)
            return result;
        }
      }
      headers = headers->next;
    }
  }

  return CURLE_OK;
}