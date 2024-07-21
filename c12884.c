static CURLcode smtp_connect(struct connectdata *conn,
                             bool *done) /* see description above */
{
  CURLcode result;
  struct smtp_conn *smtpc = &conn->proto.smtpc;
  struct SessionHandle *data = conn->data;
  struct pingpong *pp = &smtpc->pp;
  const char *path = conn->data->state.path;
  int len;
  char localhost[HOSTNAME_MAX + 1];

  *done = FALSE; /* default to not done yet */

  /* If there already is a protocol-specific struct allocated for this
     sessionhandle, deal with it */
  Curl_reset_reqproto(conn);

  result = smtp_init(conn);
  if(CURLE_OK != result)
    return result;

  /* We always support persistent connections on smtp */
  conn->bits.close = FALSE;

  pp->response_time = RESP_TIMEOUT; /* set default response time-out */
  pp->statemach_act = smtp_statemach_act;
  pp->endofresp = smtp_endofresp;
  pp->conn = conn;

  if(conn->bits.tunnel_proxy && conn->bits.httpproxy) {
    /* for SMTP over HTTP proxy */
    struct HTTP http_proxy;
    struct FTP *smtp_save;

    /* BLOCKING */
    /* We want "seamless" SMTP operations through HTTP proxy tunnel */

    /* Curl_proxyCONNECT is based on a pointer to a struct HTTP at the member
     * conn->proto.http; we want SMTP through HTTP and we have to change the
     * member temporarily for connecting to the HTTP proxy. After
     * Curl_proxyCONNECT we have to set back the member to the original struct
     * SMTP pointer
     */
    smtp_save = data->state.proto.smtp;
    memset(&http_proxy, 0, sizeof(http_proxy));
    data->state.proto.http = &http_proxy;

    result = Curl_proxyCONNECT(conn, FIRSTSOCKET,
                               conn->host.name, conn->remote_port);

    data->state.proto.smtp = smtp_save;

    if(CURLE_OK != result)
      return result;
  }

  if((conn->handler->protocol & CURLPROTO_SMTPS) &&
      data->state.used_interface != Curl_if_multi) {
    /* SMTPS is simply smtp with SSL for the control channel */
    /* now, perform the SSL initialization for this socket */
    result = Curl_ssl_connect(conn, FIRSTSOCKET);
    if(result)
      return result;
  }

  Curl_pp_init(pp); /* init the response reader stuff */

  pp->response_time = RESP_TIMEOUT; /* set default response time-out */
  pp->statemach_act = smtp_statemach_act;
  pp->endofresp = smtp_endofresp;
  pp->conn = conn;

  if(!*path) {
    if(!Curl_gethostname(localhost, sizeof localhost))
      path = localhost;
    else
      path = "localhost";
  }

  /* url decode the path and use it as domain with EHLO */
  smtpc->domain = curl_easy_unescape(conn->data, path, 0, &len);
  if(!smtpc->domain)
    return CURLE_OUT_OF_MEMORY;

  /* When we connect, we start in the state where we await the server greeting
   */
  state(conn, SMTP_SERVERGREET);

  if(data->state.used_interface == Curl_if_multi)
    result = smtp_multi_statemach(conn, done);
  else {
    result = smtp_easy_statemach(conn);
    if(!result)
      *done = TRUE;
  }

  return result;
}