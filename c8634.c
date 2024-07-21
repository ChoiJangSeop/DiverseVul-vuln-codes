int
http_message_needs_eof (const http_parser *parser)
{
  if (parser->type == HTTP_REQUEST) {
    return 0;
  }

  /* See RFC 2616 section 4.4 */
  if (parser->status_code / 100 == 1 || /* 1xx e.g. Continue */
      parser->status_code == 204 ||     /* No Content */
      parser->status_code == 304 ||     /* Not Modified */
      parser->flags & F_SKIPBODY) {     /* response to a HEAD request */
    return 0;
  }

  if ((parser->flags & F_CHUNKED) || parser->content_length != ULLONG_MAX) {
    return 0;
  }

  return 1;