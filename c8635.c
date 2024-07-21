int llhttp__after_headers_complete(llhttp_t* parser, const char* p,
                                   const char* endp) {
  int hasBody;

  hasBody = parser->flags & F_CHUNKED || parser->content_length > 0;
  if (parser->upgrade && (parser->method == HTTP_CONNECT ||
                          (parser->flags & F_SKIPBODY) || !hasBody)) {
    /* Exit, the rest of the message is in a different protocol. */
    return 1;
  }

  if (parser->flags & F_SKIPBODY) {
    return 0;
  } else if (parser->flags & F_CHUNKED) {
    /* chunked encoding - ignore Content-Length header */
    return 2;
  } else {
    if (!(parser->flags & F_CONTENT_LENGTH)) {
      if (!llhttp_message_needs_eof(parser)) {
        /* Assume content-length 0 - read the next */
        return 0;
      } else {
        /* Read body until EOF */
        return 4;
      }
    } else if (parser->content_length == 0) {
      /* Content-Length header given but zero: Content-Length: 0\r\n */
      return 0;
    } else {
      /* Content-Length header given and non-zero */
      return 3;
    }
  }
}