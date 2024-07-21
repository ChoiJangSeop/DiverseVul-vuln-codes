CURLcode Curl_output_negotiate(struct connectdata *conn, bool proxy)
{
  struct negotiatedata *neg_ctx = proxy?&conn->data->state.proxyneg:
    &conn->data->state.negotiate;
  char *encoded = NULL;
  size_t len = 0;
  char *userp;
  CURLcode error;
  OM_uint32 discard_st;

  error = Curl_base64_encode(conn->data,
                             neg_ctx->output_token.value,
                             neg_ctx->output_token.length,
                             &encoded, &len);
  if(error) {
    gss_release_buffer(&discard_st, &neg_ctx->output_token);
    neg_ctx->output_token.value = NULL;
    neg_ctx->output_token.length = 0;
    return error;
  }

  if(!encoded || !len) {
    gss_release_buffer(&discard_st, &neg_ctx->output_token);
    neg_ctx->output_token.value = NULL;
    neg_ctx->output_token.length = 0;
    return CURLE_REMOTE_ACCESS_DENIED;
  }

  userp = aprintf("%sAuthorization: %s %s\r\n", proxy ? "Proxy-" : "",
                  neg_ctx->protocol, encoded);
  if(proxy) {
    Curl_safefree(conn->allocptr.proxyuserpwd);
    conn->allocptr.proxyuserpwd = userp;
  }
  else {
    Curl_safefree(conn->allocptr.userpwd);
    conn->allocptr.userpwd = userp;
  }

  Curl_safefree(encoded);
  Curl_cleanup_negotiate(conn->data);

  return (userp == NULL) ? CURLE_OUT_OF_MEMORY : CURLE_OK;
}