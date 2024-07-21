connect_to_server (CamelService *service,
                   GCancellable *cancellable,
                   GError **error)
{
	CamelSmtpTransport *transport = CAMEL_SMTP_TRANSPORT (service);
	CamelNetworkSettings *network_settings;
	CamelNetworkSecurityMethod method;
	CamelSettings *settings;
	CamelStream *stream, *ostream = NULL;
	CamelStreamBuffer *istream = NULL;
	GIOStream *base_stream;
	GIOStream *tls_stream;
	gchar *respbuf = NULL;
	gboolean success = TRUE;
	gboolean ignore_8bitmime;
	gchar *host;

	if (!CAMEL_SERVICE_CLASS (camel_smtp_transport_parent_class)->
		connect_sync (service, cancellable, error))
		return FALSE;

	/* set some smtp transport defaults */
	transport->flags = 0;
	transport->authtypes = NULL;

	settings = camel_service_ref_settings (service);

	network_settings = CAMEL_NETWORK_SETTINGS (settings);
	host = camel_network_settings_dup_host (network_settings);
	method = camel_network_settings_get_security_method (network_settings);

	g_object_unref (settings);

	base_stream = camel_network_service_connect_sync (
		CAMEL_NETWORK_SERVICE (service), cancellable, error);

	if (base_stream != NULL) {
		/* get the localaddr - needed later by smtp_helo */
		transport->local_address =
			g_socket_connection_get_local_address (
			G_SOCKET_CONNECTION (base_stream), NULL);

		stream = camel_stream_new (base_stream);
		g_object_unref (base_stream);
	} else {
		success = FALSE;
		goto exit;
	}

	transport->connected = TRUE;

	g_mutex_lock (&transport->stream_lock);

	transport->ostream = stream;
	transport->istream = CAMEL_STREAM_BUFFER (camel_stream_buffer_new (
		stream, CAMEL_STREAM_BUFFER_READ));

	istream = g_object_ref (transport->istream);
	ostream = g_object_ref (transport->ostream);

	g_mutex_unlock (&transport->stream_lock);

	/* Read the greeting, note whether the server is ESMTP or not. */
	do {
		/* Check for "220" */
		g_free (respbuf);
		respbuf = camel_stream_buffer_read_line (istream, cancellable, error);
		d (fprintf (stderr, "[SMTP] received: %s\n", respbuf ? respbuf : "(null)"));
		if (respbuf == NULL) {
			g_prefix_error (error, _("Welcome response error: "));
			transport->connected = FALSE;
			success = FALSE;
			goto exit;
		}
		if (strncmp (respbuf, "220", 3)) {
			smtp_set_error (transport, istream, respbuf, cancellable, error);
			g_prefix_error (error, _("Welcome response error: "));
			g_free (respbuf);
			success = FALSE;
			goto exit;
		}
	} while (*(respbuf+3) == '-'); /* if we got "220-" then loop again */
	g_free (respbuf);

	ignore_8bitmime = host && camel_strstrcase (host, "yahoo.com");

	/* Try sending EHLO */
	transport->flags |= CAMEL_SMTP_TRANSPORT_IS_ESMTP;
	if (!smtp_helo (transport, istream, ostream, ignore_8bitmime, cancellable, error)) {
		if (!transport->connected) {
			success = FALSE;
			goto exit;
		}

		/* Fall back to HELO */
		g_clear_error (error);
		transport->flags &= ~CAMEL_SMTP_TRANSPORT_IS_ESMTP;

		if (!smtp_helo (transport, istream, ostream, ignore_8bitmime, cancellable, error)) {
			success = FALSE;
			goto exit;
		}
	}

	/* Clear any EHLO/HELO exception and assume that
	 * any SMTP errors encountered were non-fatal. */
	g_clear_error (error);

	if (method != CAMEL_NETWORK_SECURITY_METHOD_STARTTLS_ON_STANDARD_PORT)
		goto exit;  /* we're done */

	if (!(transport->flags & CAMEL_SMTP_TRANSPORT_STARTTLS)) {
		g_set_error (
			error, CAMEL_ERROR, CAMEL_ERROR_GENERIC,
			_("Failed to connect to SMTP server %s in secure mode: %s"),
			host, _("STARTTLS not supported"));

		success = FALSE;
		goto exit;
	}

	d (fprintf (stderr, "[SMTP] sending: STARTTLS\r\n"));
	if (camel_stream_write (ostream, "STARTTLS\r\n", 10, cancellable, error) == -1) {
		g_prefix_error (error, _("STARTTLS command failed: "));
		success = FALSE;
		goto exit;
	}

	respbuf = NULL;

	do {
		/* Check for "220 Ready for TLS" */
		g_free (respbuf);
		respbuf = camel_stream_buffer_read_line (istream, cancellable, error);
		d (fprintf (stderr, "[SMTP] received: %s\n", respbuf ? respbuf : "(null)"));
		if (respbuf == NULL) {
			g_prefix_error (error, _("STARTTLS command failed: "));
			transport->connected = FALSE;
			success = FALSE;
			goto exit;
		}
		if (strncmp (respbuf, "220", 3) != 0) {
			smtp_set_error (transport, istream, respbuf, cancellable, error);
			g_prefix_error (error, _("STARTTLS command failed: "));
			g_free (respbuf);
			success = FALSE;
			goto exit;
		}
	} while (*(respbuf+3) == '-'); /* if we got "220-" then loop again */

	/* Okay, now toggle SSL/TLS mode */
	base_stream = camel_stream_ref_base_stream (stream);
	tls_stream = camel_network_service_starttls (
		CAMEL_NETWORK_SERVICE (service), base_stream, error);
	g_object_unref (base_stream);

	if (tls_stream != NULL) {
		camel_stream_set_base_stream (stream, tls_stream);
		g_object_unref (tls_stream);
	} else {
		g_prefix_error (
			error,
			_("Failed to connect to SMTP server %s in secure mode: "),
			host);
		success = FALSE;
		goto exit;
	}

	/* We are supposed to re-EHLO after a successful STARTTLS to
	 * re-fetch any supported extensions. */
	if (!smtp_helo (transport, istream, ostream, ignore_8bitmime, cancellable, error)) {
		success = FALSE;
	}

exit:
	g_free (host);

	if (!success) {
		transport->connected = FALSE;

		g_mutex_lock (&transport->stream_lock);

		g_clear_object (&transport->istream);
		g_clear_object (&transport->ostream);

		g_mutex_unlock (&transport->stream_lock);
	}

	g_clear_object (&istream);
	g_clear_object (&ostream);

	return success;
}