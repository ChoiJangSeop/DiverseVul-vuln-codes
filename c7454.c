static void server_handshake(pn_transport_t* transport)
{
  pni_ssl_t *ssl = transport->ssl;
  if (!ssl->protocol_detected) {
    // SChannel fails less aggressively than openssl on client hello, causing hangs
    // waiting for more bytes.  Help out here.
    pni_protocol_type_t type = pni_sniff_header(ssl->sc_inbuf, ssl->sc_in_count);
    if (type == PNI_PROTOCOL_INSUFFICIENT) {
      ssl_log(transport, "server handshake: incomplete record");
      ssl->sc_in_incomplete = true;
      return;
    } else {
      ssl->protocol_detected = true;
      if (type != PNI_PROTOCOL_SSL) {
        ssl_failed(transport, "bad client hello");
        ssl->decrypting = false;
        rewind_sc_inbuf(ssl);
        return;
      }
    }
  }

  // Feed SChannel ongoing handshake records from the client until the handshake is complete.
  ULONG ctxt_requested = ASC_REQ_STREAM | ASC_REQ_EXTENDED_ERROR;
  if (ssl->verify_mode == PN_SSL_VERIFY_PEER || ssl->verify_mode == PN_SSL_VERIFY_PEER_NAME)
    ctxt_requested |= ASC_REQ_MUTUAL_AUTH;
  ULONG ctxt_attrs;
  size_t max = 0;

  // token_buffs describe the buffer that's coming in. It should have
  // a token from the SSL client except if shutting down or renegotiating.
  bool shutdown = ssl->state == SHUTTING_DOWN;
  SecBuffer token_buffs[2];
  token_buffs[0].cbBuffer = shutdown ? 0 : ssl->sc_in_count;
  token_buffs[0].BufferType = SECBUFFER_TOKEN;
  token_buffs[0].pvBuffer = shutdown ? 0 : ssl->sc_inbuf;
  token_buffs[1].cbBuffer = 0;
  token_buffs[1].BufferType = SECBUFFER_EMPTY;
  token_buffs[1].pvBuffer = 0;
  SecBufferDesc token_buff_desc;
  token_buff_desc.ulVersion = SECBUFFER_VERSION;
  token_buff_desc.cBuffers = 2;
  token_buff_desc.pBuffers = token_buffs;

  // send_buffs will hold information to forward to the peer.
  SecBuffer send_buffs[2];
  send_buffs[0].cbBuffer = ssl->sc_out_size;
  send_buffs[0].BufferType = SECBUFFER_TOKEN;
  send_buffs[0].pvBuffer = ssl->sc_outbuf;
  send_buffs[1].cbBuffer = 0;
  send_buffs[1].BufferType = SECBUFFER_EMPTY;
  send_buffs[1].pvBuffer = 0;
  SecBufferDesc send_buff_desc;
  send_buff_desc.ulVersion = SECBUFFER_VERSION;
  send_buff_desc.cBuffers = 2;
  send_buff_desc.pBuffers = send_buffs;
  PCtxtHandle ctxt_handle_ptr = (SecIsValidHandle(&ssl->ctxt_handle)) ? &ssl->ctxt_handle : 0;

  SECURITY_STATUS status;
  {
    csguard g(&ssl->cred->cslock);
    status = AcceptSecurityContext(&ssl->cred_handle, ctxt_handle_ptr,
                               &token_buff_desc, ctxt_requested, 0, &ssl->ctxt_handle,
                               &send_buff_desc, &ctxt_attrs, NULL);
  }
  bool outbound_token = false;
  switch(status) {
  case SEC_E_INCOMPLETE_MESSAGE:
    // Not enough - get more data from the client then try again.
    // Leave input buffers untouched.
    ssl_log(transport, "server handshake: incomplete record");
    ssl->sc_in_incomplete = true;
    return;

  case SEC_I_CONTINUE_NEEDED:
    outbound_token = true;
    break;

  case SEC_E_OK:
    // Handshake complete.
    if (shutdown) {
      if (send_buffs[0].cbBuffer > 0) {
        ssl->sc_out_count = send_buffs[0].cbBuffer;
        // the token is the whole quantity to send
        ssl->network_out_pending = ssl->sc_out_count;
        ssl->network_outp = ssl->sc_outbuf;
        ssl_log(transport, "server shutdown token %d bytes", ssl->network_out_pending);
      } else {
        ssl->state = SSL_CLOSED;
      }
      // we didn't touch sc_inbuf, no need to reset
      return;
    }
    if (const char *err = tls_version_check(ssl)) {
      ssl_failed(transport, err);
      break;
    }
    // Handshake complete.

    if (ssl->verify_mode == PN_SSL_VERIFY_PEER || ssl->verify_mode == PN_SSL_VERIFY_PEER_NAME) {
      bool tracing = PN_TRACE_DRV & transport->trace;
      HRESULT ec = verify_peer(ssl, ssl->cred->trust_store, NULL, tracing);
      if (ec) {
        ssl_log_error_status(ec, "certificate verification failed");
        ssl_failed(transport, "certificate verification error");
        break;
      }
    }

    QueryContextAttributes(&ssl->ctxt_handle,
                             SECPKG_ATTR_STREAM_SIZES, &ssl->sc_sizes);
    max = ssl->sc_sizes.cbMaximumMessage + ssl->sc_sizes.cbHeader + ssl->sc_sizes.cbTrailer;
    if (max > ssl->sc_out_size) {
      ssl_log_error("Buffer size mismatch have %d, need %d", (int) ssl->sc_out_size, (int) max);
      ssl->state = SHUTTING_DOWN;
      ssl->app_input_closed = ssl->app_output_closed = PN_ERR;
      start_ssl_shutdown(transport);
      pn_do_error(transport, "amqp:connection:framing-error", "SSL Failure: buffer size");
      break;
    }

    if (send_buffs[0].cbBuffer != 0)
      outbound_token = true;

    ssl->state = RUNNING;
    ssl->max_data_size = max - ssl->sc_sizes.cbHeader - ssl->sc_sizes.cbTrailer;
    ssl_log(transport, "server handshake successful %d max record size", max);
    break;

  case SEC_I_CONTEXT_EXPIRED:
    // ended before we got going
  default:
    ssl_log(transport, "server handshake failed %d", (int) status);
    ssl_failed(transport, 0);
    break;
  }

  if (outbound_token) {
    // Successful handshake step, requiring data to be sent to peer.
    assert(ssl->network_out_pending == 0);
    ssl->sc_out_count = send_buffs[0].cbBuffer;
    // the token is the whole quantity to send
    ssl->network_out_pending = ssl->sc_out_count;
    ssl->network_outp = ssl->sc_outbuf;
    ssl_log(transport, "server handshake token %d bytes", ssl->network_out_pending);
  }

  if (token_buffs[1].BufferType == SECBUFFER_EXTRA && token_buffs[1].cbBuffer > 0 &&
      !ssl->ssl_closed) {
    // remaining data after the consumed TLS record(s)
    ssl->extra_count = token_buffs[1].cbBuffer;
    ssl->inbuf_extra = ssl->sc_inbuf + (ssl->sc_in_count - ssl->extra_count);
  }

  ssl->decrypting = false;
  rewind_sc_inbuf(ssl);
}