CURLcode Curl_output_negotiate(struct connectdata *conn, bool proxy)
{
  struct negotiatedata *neg_ctx = proxy?&conn->data->state.proxyneg:
    &conn->data->state.negotiate;
  char *encoded = NULL;
  size_t len = 0;
  char *userp;
  CURLcode error;

  error = Curl_base64_encode(conn->data,
                             (const char*)neg_ctx->output_token,
                             neg_ctx->output_token_length,
                             &encoded, &len);
  if(error)
    return error;

  if(len == 0)
    return CURLE_REMOTE_ACCESS_DENIED;

  userp = aprintf("%sAuthorization: %s %s\r\n", proxy ? "Proxy-" : "",
                  neg_ctx->protocol, encoded);

  if(proxy)
    conn->allocptr.proxyuserpwd = userp;
  else
    conn->allocptr.userpwd = userp;
  free(encoded);
  Curl_cleanup_negotiate (conn->data);
  return (userp == NULL) ? CURLE_OUT_OF_MEMORY : CURLE_OK;
}