int credential_from_url_gently(struct credential *c, const char *url,
			       int quiet)
{
	const char *at, *colon, *cp, *slash, *host, *proto_end;

	credential_clear(c);

	/*
	 * Match one of:
	 *   (1) proto://<host>/...
	 *   (2) proto://<user>@<host>/...
	 *   (3) proto://<user>:<pass>@<host>/...
	 */
	proto_end = strstr(url, "://");
	if (!proto_end)
		return 0;
	cp = proto_end + 3;
	at = strchr(cp, '@');
	colon = strchr(cp, ':');
	slash = strchrnul(cp, '/');

	if (!at || slash <= at) {
		/* Case (1) */
		host = cp;
	}
	else if (!colon || at <= colon) {
		/* Case (2) */
		c->username = url_decode_mem(cp, at - cp);
		host = at + 1;
	} else {
		/* Case (3) */
		c->username = url_decode_mem(cp, colon - cp);
		c->password = url_decode_mem(colon + 1, at - (colon + 1));
		host = at + 1;
	}

	if (proto_end - url > 0)
		c->protocol = xmemdupz(url, proto_end - url);
	c->host = url_decode_mem(host, slash - host);
	/* Trim leading and trailing slashes from path */
	while (*slash == '/')
		slash++;
	if (*slash) {
		char *p;
		c->path = url_decode(slash);
		p = c->path + strlen(c->path) - 1;
		while (p > c->path && *p == '/')
			*p-- = '\0';
	}

	if (check_url_component(url, quiet, "username", c->username) < 0 ||
	    check_url_component(url, quiet, "password", c->password) < 0 ||
	    check_url_component(url, quiet, "protocol", c->protocol) < 0 ||
	    check_url_component(url, quiet, "host", c->host) < 0 ||
	    check_url_component(url, quiet, "path", c->path) < 0)
		return -1;

	return 0;
}