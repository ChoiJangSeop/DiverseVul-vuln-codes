static CURLcode pop3_parse_url_path(struct connectdata *conn)
{
  /* the pop3 struct is already inited in pop3_connect() */
  struct pop3_conn *pop3c = &conn->proto.pop3c;
  struct SessionHandle *data = conn->data;
  const char *path = data->state.path;

  /* url decode the path and use this mailbox */
  pop3c->mailbox = curl_easy_unescape(data, path, 0, NULL);
  if(!pop3c->mailbox)
    return CURLE_OUT_OF_MEMORY;

  return CURLE_OK;
}