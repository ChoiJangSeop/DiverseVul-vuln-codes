static int cmd_starttls_start(struct smtp_server_connection *conn)
{
	const struct smtp_server_callbacks *callbacks = conn->callbacks;

	e_debug(conn->event, "Starting TLS");

	if (callbacks != NULL && callbacks->conn_start_tls != NULL) {
		struct smtp_server_connection *tmp_conn = conn;
		struct istream *input = conn->conn.input;
		struct ostream *output = conn->conn.output;
		int ret;

		smtp_server_connection_ref(tmp_conn);
		ret = callbacks->conn_start_tls(tmp_conn->context,
			&input, &output);
		if (!smtp_server_connection_unref(&tmp_conn) || ret < 0)
			return -1;

		smtp_server_connection_set_ssl_streams(conn, input, output);
	} else if (smtp_server_connection_ssl_init(conn) < 0) {
		smtp_server_connection_close(&conn,
			"SSL Initialization failed");
		return -1;
	}

	/* RFC 3207, Section 4.2:

	   Upon completion of the TLS handshake, the SMTP protocol is reset to
	   the initial state (the state in SMTP after a server issues a 220
	   service ready greeting). The server MUST discard any knowledge
	   obtained from the client, such as the argument to the EHLO command,
	   which was not obtained from the TLS negotiation itself.
	*/
	smtp_server_connection_clear(conn);
	smtp_server_connection_input_unlock(conn);

	return 0;
}