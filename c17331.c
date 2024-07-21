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
    /* chunked encoding - ignore Content-Length header, prepare for a chunk */
    return 2;
  } else if (parser->flags & F_TRANSFER_ENCODING) {
    if (parser->type == HTTP_REQUEST &&
        (parser->lenient_flags & LENIENT_CHUNKED_LENGTH) == 0) {
      /* RFC 7230 3.3.3 */

      /* If a Transfer-Encoding header field
       * is present in a request and the chunked transfer coding is not
       * the final encoding, the message body length cannot be determined
       * reliably; the server MUST respond with the 400 (Bad Request)
       * status code and then close the connection.
       */
      return 5;
    } else {
      /* RFC 7230 3.3.3 */

      /* If a Transfer-Encoding header field is present in a response and
       * the chunked transfer coding is not the final encoding, the
       * message body length is determined by reading the connection until
       * it is closed by the server.
       */
      return 4;
    }
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