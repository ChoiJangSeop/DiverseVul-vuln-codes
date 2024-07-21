PHPAPI php_url *php_url_parse_ex2(char const *str, size_t length, zend_bool *has_port)
{
	char port_buf[6];
	php_url *ret = ecalloc(1, sizeof(php_url));
	char const *s, *e, *p, *pp, *ue;

	*has_port = 0;
	s = str;
	ue = s + length;

	/* parse scheme */
	if ((e = memchr(s, ':', length)) && e != s) {
		/* validate scheme */
		p = s;
		while (p < e) {
			/* scheme = 1*[ lowalpha | digit | "+" | "-" | "." ] */
			if (!isalpha(*p) && !isdigit(*p) && *p != '+' && *p != '.' && *p != '-') {
				if (e + 1 < ue && e < binary_strcspn(s, ue, "?#")) {
					goto parse_port;
				} else if (s + 1 < ue && *s == '/' && *(s + 1) == '/') { /* relative-scheme URL */
					s += 2;
					e = 0;
					goto parse_host;
				} else {
					goto just_path;
				}
			}
			p++;
		}

		if (e + 1 == ue) { /* only scheme is available */
			ret->scheme = zend_string_init(s, (e - s), 0);
			php_replace_controlchars_ex(ZSTR_VAL(ret->scheme), ZSTR_LEN(ret->scheme));
			return ret;
		}

		/*
		 * certain schemas like mailto: and zlib: may not have any / after them
		 * this check ensures we support those.
		 */
		if (*(e+1) != '/') {
			/* check if the data we get is a port this allows us to
			 * correctly parse things like a.com:80
			 */
			p = e + 1;
			while (p < ue && isdigit(*p)) {
				p++;
			}

			if ((p == ue || *p == '/') && (p - e) < 7) {
				goto parse_port;
			}

			ret->scheme = zend_string_init(s, (e-s), 0);
			php_replace_controlchars_ex(ZSTR_VAL(ret->scheme), ZSTR_LEN(ret->scheme));

			s = e + 1;
			goto just_path;
		} else {
			ret->scheme = zend_string_init(s, (e-s), 0);
			php_replace_controlchars_ex(ZSTR_VAL(ret->scheme), ZSTR_LEN(ret->scheme));

			if (e + 2 < ue && *(e + 2) == '/') {
				s = e + 3;
				if (zend_string_equals_literal_ci(ret->scheme, "file")) {
					if (e + 3 < ue && *(e + 3) == '/') {
						/* support windows drive letters as in:
						   file:///c:/somedir/file.txt
						*/
						if (e + 5 < ue && *(e + 5) == ':') {
							s = e + 4;
						}
						goto just_path;
					}
				}
			} else {
				s = e + 1;
				goto just_path;
			}
		}
	} else if (e) { /* no scheme; starts with colon: look for port */
		parse_port:
		p = e + 1;
		pp = p;

		while (pp < ue && pp - p < 6 && isdigit(*pp)) {
			pp++;
		}

		if (pp - p > 0 && pp - p < 6 && (pp == ue || *pp == '/')) {
			zend_long port;
			char *end;
			memcpy(port_buf, p, (pp - p));
			port_buf[pp - p] = '\0';
			port = ZEND_STRTOL(port_buf, &end, 10);
			if (port >= 0 && port <= 65535 && end != port_buf) {
				*has_port = 1;
				ret->port = (unsigned short) port;
				if (s + 1 < ue && *s == '/' && *(s + 1) == '/') { /* relative-scheme URL */
				    s += 2;
				}
			} else {
				php_url_free(ret);
				return NULL;
			}
		} else if (p == pp && pp == ue) {
			php_url_free(ret);
			return NULL;
		} else if (s + 1 < ue && *s == '/' && *(s + 1) == '/') { /* relative-scheme URL */
			s += 2;
		} else {
			goto just_path;
		}
	} else if (s + 1 < ue && *s == '/' && *(s + 1) == '/') { /* relative-scheme URL */
		s += 2;
	} else {
		goto just_path;
	}

parse_host:
	e = binary_strcspn(s, ue, "/?#");

	/* check for login and password */
	if ((p = zend_memrchr(s, '@', (e-s)))) {
		if ((pp = memchr(s, ':', (p-s)))) {
			ret->user = zend_string_init(s, (pp-s), 0);
			php_replace_controlchars_ex(ZSTR_VAL(ret->user), ZSTR_LEN(ret->user));

			pp++;
			ret->pass = zend_string_init(pp, (p-pp), 0);
			php_replace_controlchars_ex(ZSTR_VAL(ret->pass), ZSTR_LEN(ret->pass));
		} else {
            if (!is_userinfo_valid(s, p-s)) {
                goto check_port;
            }
            ret->user = zend_string_init(s, (p-s), 0);
			php_replace_controlchars_ex(ZSTR_VAL(ret->user), ZSTR_LEN(ret->user));
		}

		s = p + 1;
	}

check_port:
	/* check for port */
	if (s < ue && *s == '[' && *(e-1) == ']') {
		/* Short circuit portscan,
		   we're dealing with an
		   IPv6 embedded address */
		p = NULL;
	} else {
		p = zend_memrchr(s, ':', (e-s));
	}

	if (p) {
		if (!ret->port) {
			p++;
			if (e-p > 5) { /* port cannot be longer then 5 characters */
				php_url_free(ret);
				return NULL;
			} else if (e - p > 0) {
				zend_long port;
				char *end;
				memcpy(port_buf, p, (e - p));
				port_buf[e - p] = '\0';
				port = ZEND_STRTOL(port_buf, &end, 10);
				if (port >= 0 && port <= 65535 && end != port_buf) {
					*has_port = 1;
					ret->port = (unsigned short)port;
				} else {
					php_url_free(ret);
					return NULL;
				}
			}
			p--;
		}
	} else {
		p = e;
	}

	/* check if we have a valid host, if we don't reject the string as url */
	if ((p-s) < 1) {
		php_url_free(ret);
		return NULL;
	}

	ret->host = zend_string_init(s, (p-s), 0);
	php_replace_controlchars_ex(ZSTR_VAL(ret->host), ZSTR_LEN(ret->host));

	if (e == ue) {
		return ret;
	}

	s = e;

	just_path:

	e = ue;
	p = memchr(s, '#', (e - s));
	if (p) {
		p++;
		if (p < e) {
			ret->fragment = zend_string_init(p, (e - p), 0);
			php_replace_controlchars_ex(ZSTR_VAL(ret->fragment), ZSTR_LEN(ret->fragment));
		}
		e = p-1;
	}

	p = memchr(s, '?', (e - s));
	if (p) {
		p++;
		if (p < e) {
			ret->query = zend_string_init(p, (e - p), 0);
			php_replace_controlchars_ex(ZSTR_VAL(ret->query), ZSTR_LEN(ret->query));
		}
		e = p-1;
	}

	if (s < e || s == ue) {
		ret->path = zend_string_init(s, (e - s), 0);
		php_replace_controlchars_ex(ZSTR_VAL(ret->path), ZSTR_LEN(ret->path));
	}

	return ret;
}