R_API char *r_socket_http_get(const char *url, int *code, int *rlen) {
	char *curl_env = r_sys_getenv ("R2_CURL");
	if (curl_env && *curl_env) {
		char *encoded_url = r_str_escape (url);
		char *res = r_sys_cmd_strf ("curl '%s'", encoded_url);
		free (encoded_url);
		if (res) {
			if (code) {
				*code = 200;
			}
			if (rlen) {
				*rlen = strlen (res);
			}
		}
		free (curl_env);
		return res;
	}
	free (curl_env);
	RSocket *s;
	int ssl = r_str_startswith (url, "https://");
	char *response, *host, *path, *port = "80";
	char *uri = strdup (url);
	if (!uri) {
		return NULL;
	}

	if (code) {
		*code = 0;
	}
	if (rlen) {
		*rlen = 0;
	}
	host = strstr (uri, "://");
	if (!host) {
		free (uri);
		eprintf ("r_socket_http_get: Invalid URI");
		return NULL;
	}
	host += 3;
	port = strchr (host, ':');
	if (!port) {
		port = ssl? "443": "80";
		path = host;
	} else {
		*port++ = 0;
		path = port;
	}
	path = strchr (path, '/');
	if (!path) {
		path = "";
	} else {
		*path++ = 0;
	}
	s = r_socket_new (ssl);
	if (!s) {
		eprintf ("r_socket_http_get: Cannot create socket\n");
		free (uri);
		return NULL;
	}
	if (r_socket_connect_tcp (s, host, port, 0)) {
		r_socket_printf (s,
				"GET /%s HTTP/1.1\r\n"
				"User-Agent: radare2 "R2_VERSION"\r\n"
				"Accept: */*\r\n"
				"Host: %s:%s\r\n"
				"\r\n", path, host, port);
		response = r_socket_http_answer (s, code, rlen);
	} else {
		eprintf ("Cannot connect to %s:%s\n", host, port);
		response = NULL;
	}
	free (uri);
	r_socket_free (s);
	return response;
}