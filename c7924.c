static CURLcode read_data(struct connectdata *conn,
                          curl_socket_t fd,
                          struct krb5buffer *buf)
{
  int len;
  void *tmp = NULL;
  CURLcode result;

  result = socket_read(fd, &len, sizeof(len));
  if(result)
    return result;

  if(len) {
    /* only realloc if there was a length */
    len = ntohl(len);
    tmp = Curl_saferealloc(buf->data, len);
  }
  if(tmp == NULL)
    return CURLE_OUT_OF_MEMORY;

  buf->data = tmp;
  result = socket_read(fd, buf->data, len);
  if(result)
    return result;
  buf->size = conn->mech->decode(conn->app_data, buf->data, len,
                                 conn->data_prot, conn);
  buf->index = 0;
  return CURLE_OK;
}