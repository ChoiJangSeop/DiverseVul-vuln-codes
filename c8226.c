soup_auth_ntlm_get_connection_authorization (SoupConnectionAuth *auth,
					     SoupMessage        *msg,
					     gpointer            state)
{
	SoupAuthNTLM *auth_ntlm = SOUP_AUTH_NTLM (auth);
	SoupAuthNTLMPrivate *priv = soup_auth_ntlm_get_instance_private (auth_ntlm);
	SoupNTLMConnectionState *conn = state;
	char *header = NULL;

	switch (conn->state) {
	case SOUP_NTLM_NEW:
#ifdef USE_NTLM_AUTH
		if (sso_ntlm_initiate (priv)) {
			header = sso_ntlm_response (priv, "YR\n", conn->state);
			if (header) {
				if (g_ascii_strcasecmp (header, "PW") != 0) {
					conn->state = SOUP_NTLM_SENT_REQUEST;
					break;
				} else {
					g_free (header);
					header = NULL;
					priv->sso_available = FALSE;
				}
			} else {
				g_debug ("NTLM single-sign-on using %s failed", NTLM_AUTH);
			}
		}
		/* If NTLM single-sign-on fails, go back to original
		 * request handling process.
		 */
#endif
		header = soup_ntlm_request ();
		conn->state = SOUP_NTLM_SENT_REQUEST;
		break;
	case SOUP_NTLM_RECEIVED_CHALLENGE:
		if (conn->response_header) {
			header = conn->response_header;
			conn->response_header = NULL;
		} else {
			header = soup_ntlm_response (conn->nonce,
						     priv->username,
						     priv->nt_hash,
						     priv->lm_hash,
						     NULL,
						     priv->domain,
						     conn->ntlmv2_session);
		}
		g_clear_pointer (&conn->nonce, g_free);
		conn->state = SOUP_NTLM_SENT_RESPONSE;

		if (priv->password_state != SOUP_NTLM_PASSWORD_ACCEPTED) {
			/* We need to know if this worked */
			g_signal_connect (msg, "got-headers",
					  G_CALLBACK (got_final_auth_result),
					  auth);
		}
		break;
#ifdef USE_NTLM_AUTH
	case SOUP_NTLM_SSO_FAILED:
		/* Restart request without SSO */
		g_debug ("NTLM single-sign-on by using %s failed", NTLM_AUTH);
		priv->sso_available = FALSE;
		header = soup_ntlm_request ();
		conn->state = SOUP_NTLM_SENT_REQUEST;
		break;
#endif
	default:
		break;
	}

	return header;
}