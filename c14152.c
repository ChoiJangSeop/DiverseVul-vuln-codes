CURLcode Curl_http_host(struct Curl_easy *data, struct connectdata *conn)
{
  const char *ptr;
  if(!data->state.this_is_a_follow) {
    /* Free to avoid leaking memory on multiple requests*/
    free(data->state.first_host);

    data->state.first_host = strdup(conn->host.name);
    if(!data->state.first_host)
      return CURLE_OUT_OF_MEMORY;

    data->state.first_remote_port = conn->remote_port;
  }
  Curl_safefree(data->state.aptr.host);

  ptr = Curl_checkheaders(data, STRCONST("Host"));
  if(ptr && (!data->state.this_is_a_follow ||
             strcasecompare(data->state.first_host, conn->host.name))) {
#if !defined(CURL_DISABLE_COOKIES)
    /* If we have a given custom Host: header, we extract the host name in
       order to possibly use it for cookie reasons later on. We only allow the
       custom Host: header if this is NOT a redirect, as setting Host: in the
       redirected request is being out on thin ice. Except if the host name
       is the same as the first one! */
    char *cookiehost = Curl_copy_header_value(ptr);
    if(!cookiehost)
      return CURLE_OUT_OF_MEMORY;
    if(!*cookiehost)
      /* ignore empty data */
      free(cookiehost);
    else {
      /* If the host begins with '[', we start searching for the port after
         the bracket has been closed */
      if(*cookiehost == '[') {
        char *closingbracket;
        /* since the 'cookiehost' is an allocated memory area that will be
           freed later we cannot simply increment the pointer */
        memmove(cookiehost, cookiehost + 1, strlen(cookiehost) - 1);
        closingbracket = strchr(cookiehost, ']');
        if(closingbracket)
          *closingbracket = 0;
      }
      else {
        int startsearch = 0;
        char *colon = strchr(cookiehost + startsearch, ':');
        if(colon)
          *colon = 0; /* The host must not include an embedded port number */
      }
      Curl_safefree(data->state.aptr.cookiehost);
      data->state.aptr.cookiehost = cookiehost;
    }
#endif

    if(strcmp("Host:", ptr)) {
      data->state.aptr.host = aprintf("Host:%s\r\n", &ptr[5]);
      if(!data->state.aptr.host)
        return CURLE_OUT_OF_MEMORY;
    }
    else
      /* when clearing the header */
      data->state.aptr.host = NULL;
  }
  else {
    /* When building Host: headers, we must put the host name within
       [brackets] if the host name is a plain IPv6-address. RFC2732-style. */
    const char *host = conn->host.name;

    if(((conn->given->protocol&CURLPROTO_HTTPS) &&
        (conn->remote_port == PORT_HTTPS)) ||
       ((conn->given->protocol&CURLPROTO_HTTP) &&
        (conn->remote_port == PORT_HTTP)) )
      /* if(HTTPS on port 443) OR (HTTP on port 80) then don't include
         the port number in the host string */
      data->state.aptr.host = aprintf("Host: %s%s%s\r\n",
                                    conn->bits.ipv6_ip?"[":"",
                                    host,
                                    conn->bits.ipv6_ip?"]":"");
    else
      data->state.aptr.host = aprintf("Host: %s%s%s:%d\r\n",
                                    conn->bits.ipv6_ip?"[":"",
                                    host,
                                    conn->bits.ipv6_ip?"]":"",
                                    conn->remote_port);

    if(!data->state.aptr.host)
      /* without Host: we can't make a nice request */
      return CURLE_OUT_OF_MEMORY;
  }
  return CURLE_OK;
}