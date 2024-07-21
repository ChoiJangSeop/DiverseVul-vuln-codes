gint session_start_tls(Session *session)
{
	gboolean nb_mode;

	nb_mode = sock_is_nonblocking_mode(session->sock);

	session->sock->ssl_cert_auto_accept = session->ssl_cert_auto_accept;
	session->sock->gnutls_priority = session->gnutls_priority;
	session->sock->use_tls_sni = session->use_tls_sni;

	if (nb_mode)
		sock_set_nonblocking_mode(session->sock, FALSE);

	if (!ssl_init_socket(session->sock)) {
		g_warning("couldn't start STARTTLS session.");
		if (nb_mode)
			sock_set_nonblocking_mode(session->sock, session->nonblocking);
		return -1;
	}

	if (nb_mode)
		sock_set_nonblocking_mode(session->sock, session->nonblocking);

	return 0;
}