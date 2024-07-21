int MonClient::handle_auth_request(
  Connection *con,
  AuthConnectionMeta *auth_meta,
  bool more,
  uint32_t auth_method,
  const ceph::buffer::list& payload,
  ceph::buffer::list *reply)
{
  if (payload.length() == 0) {
    // for some channels prior to nautilus (osd heartbeat), we
    // tolerate the lack of an authorizer.
    if (!con->get_messenger()->require_authorizer) {
      handle_authentication_dispatcher->ms_handle_authentication(con);
      return 1;
    }
    return -EACCES;
  }
  auth_meta->auth_mode = payload[0];
  if (auth_meta->auth_mode < AUTH_MODE_AUTHORIZER ||
      auth_meta->auth_mode > AUTH_MODE_AUTHORIZER_MAX) {
    return -EACCES;
  }
  AuthAuthorizeHandler *ah = get_auth_authorize_handler(con->get_peer_type(),
							auth_method);
  if (!ah) {
    lderr(cct) << __func__ << " no AuthAuthorizeHandler found for auth method "
	       << auth_method << dendl;
    return -EOPNOTSUPP;
  }

  auto ac = &auth_meta->authorizer_challenge;
  if (!HAVE_FEATURE(con->get_features(), CEPHX_V2)) {
    if (cct->_conf->cephx_service_require_version >= 2) {
      ldout(cct,10) << __func__ << " client missing CEPHX_V2 ("
		    << "cephx_service_requre_version = "
		    << cct->_conf->cephx_service_require_version << ")" << dendl;
      return -EACCES;
    }
    ac = nullptr;
  }

  bool was_challenge = (bool)auth_meta->authorizer_challenge;
  bool isvalid = ah->verify_authorizer(
    cct,
    *rotating_secrets,
    payload,
    auth_meta->get_connection_secret_length(),
    reply,
    &con->peer_name,
    &con->peer_global_id,
    &con->peer_caps_info,
    &auth_meta->session_key,
    &auth_meta->connection_secret,
    ac);
  if (isvalid) {
    handle_authentication_dispatcher->ms_handle_authentication(con);
    return 1;
  }
  if (!more && !was_challenge && auth_meta->authorizer_challenge) {
    ldout(cct,10) << __func__ << " added challenge on " << con << dendl;
    return 0;
  }
  ldout(cct,10) << __func__ << " bad authorizer on " << con << dendl;
  // discard old challenge
  auth_meta->authorizer_challenge.reset();
  return -EACCES;
}