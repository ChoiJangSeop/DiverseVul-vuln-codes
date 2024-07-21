R_API char *r_socket_http_post (const char *url, const char *data, int *code, int *rlen) {
	RSocket *s;
	bool ssl = r_str_startswith (url, "https://");
	char *uri = strdup (url);
	if (!uri) {
		return NULL;
	}

	char *host = strstr (uri, "://");
	if (!host) {
		free (uri);
		printf ("Invalid URI");
		return NULL;
	}
	host += 3;
	char *port = strchr (host, ':');
	if (!port) {
		port = (ssl)? "443": "80";
	} else {
		*port++ = 0;
	}
	char *path = strchr (host, '/');
	if (!path) {
		path = "";
	} else {
		*path++ = 0;
	}
	s = r_socket_new (ssl);
	if (!s) {
		printf ("Cannot create socket\n");
		free (uri);
		return NULL;
	}
	if (!r_socket_connect_tcp (s, host, port, 0)) {
		eprintf ("Cannot connect to %s:%s\n", host, port);
		free (uri);
		return NULL;
	}
	/* Send */
	r_socket_printf (s,
			"POST /%s HTTP/1.0\r\n"
			"User-Agent: radare2 "R2_VERSION"\r\n"
			"Accept: */*\r\n"
			"Host: %s\r\n"
			"Content-Length: %i\r\n"
			"Content-Type: application/x-www-form-urlencoded\r\n"
			"\r\n", path, host, (int)strlen (data));
	free (uri);
	r_socket_write (s, (void *)data, strlen (data));
	return r_socket_http_answer (s, code, rlen);
}