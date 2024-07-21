static void sasl_step(IRC_SERVER_REC *server, const char *data, const char *from)
{
	GString *req = NULL;

	/* Stop the timer */
	if (server->sasl_timeout != 0) {
		g_source_remove(server->sasl_timeout);
		server->sasl_timeout = 0;
	}

	if (!sasl_reassemble_incoming(server, data, &req)) {
		sasl_step_fail(server);
		return;
	}

	if (req != NULL) {
		sasl_step_complete(server, req);
		g_string_free(req, TRUE);
	}

	/* We expect a response within a reasonable time */
	server->sasl_timeout = g_timeout_add(SASL_TIMEOUT, (GSourceFunc) sasl_timeout, server);
}