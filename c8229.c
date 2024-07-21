soup_auth_ntlm_update_connection (SoupConnectionAuth *auth, SoupMessage *msg,
				  const char *auth_header, gpointer state)
{
	SoupAuthNTLM *auth_ntlm = SOUP_AUTH_NTLM (auth);
	SoupAuthNTLMPrivate *priv = soup_auth_ntlm_get_instance_private (auth_ntlm);
	SoupNTLMConnectionState *conn = state;
	gboolean success = TRUE;

	/* Note that we only return FALSE if some sort of parsing error
	 * occurs. Otherwise, the SoupAuth is still reusable (though it may
	 * no longer be _ready or _authenticated).
	 */

	if (!g_str_has_prefix (auth_header, "NTLM"))
		return FALSE;

	if (conn->state > SOUP_NTLM_SENT_REQUEST) {
		if (priv->password_state == SOUP_NTLM_PASSWORD_ACCEPTED) {
			/* We know our password is correct, so a 401
			 * means "permission denied". The code can't deal
			 * with re-authenticating correctly, so make sure
			 * we don't try.
			 */
			conn->state = SOUP_NTLM_FAILED;
			if (soup_message_is_keepalive (msg)) {
				soup_message_headers_append (msg->response_headers,
							     "Connection", "close");
			}
			return TRUE;
		}

#ifdef USE_NTLM_AUTH
		if (priv->sso_available) {
			conn->state = SOUP_NTLM_SSO_FAILED;
			priv->password_state = SOUP_NTLM_PASSWORD_NONE;
		} else {
#endif
			conn->state = SOUP_NTLM_FAILED;
			priv->password_state = SOUP_NTLM_PASSWORD_REJECTED;
#ifdef USE_NTLM_AUTH
		}
#endif
		return TRUE;
	}

	if (conn->state == SOUP_NTLM_NEW && !auth_header[4])
		return TRUE;

	if (!auth_header[4] || !auth_header[5]) {
		conn->state = SOUP_NTLM_FAILED;
		return FALSE;
	}

	if (!soup_ntlm_parse_challenge (auth_header + 5, &conn->nonce,
					priv->domain ? NULL : &priv->domain,
					&conn->ntlmv2_session)) {
		conn->state = SOUP_NTLM_FAILED;
		return FALSE;
	}

#ifdef USE_NTLM_AUTH
	if (priv->sso_available && conn->state == SOUP_NTLM_SENT_REQUEST) {
		char *input, *response;

		/* Re-Initiate ntlm_auth process in case it was closed/killed abnormally */
		if (!sso_ntlm_initiate (priv)) {
			conn->state = SOUP_NTLM_SSO_FAILED;
			success = FALSE;
			goto out;
		}

		input = g_strdup_printf ("TT %s\n", auth_header + 5);
		response = sso_ntlm_response (priv, input, conn->state);
		sso_ntlm_close (priv);
		g_free (input);

		if (!response) {
			conn->state = SOUP_NTLM_SSO_FAILED;
			success = FALSE;
		} else if (!g_ascii_strcasecmp (response, "PW")) {
			conn->state = SOUP_NTLM_SSO_FAILED;
			priv->sso_available = FALSE;
			g_free (response);
		} else {
			conn->response_header = response;
			if (priv->password_state != SOUP_NTLM_PASSWORD_ACCEPTED)
				priv->password_state = SOUP_NTLM_PASSWORD_PROVIDED;
		}
	}
 out:
#endif

	if (conn->state == SOUP_NTLM_SENT_REQUEST)
		conn->state = SOUP_NTLM_RECEIVED_CHALLENGE;

	g_object_set (G_OBJECT (auth),
		      SOUP_AUTH_REALM, priv->domain,
		      SOUP_AUTH_HOST, soup_message_get_uri (msg)->host,
		      NULL);
	return success;
}