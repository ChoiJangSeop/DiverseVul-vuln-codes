process_request(isc_httpd_t *httpd, isc_region_t *region, size_t *buflen) {
	char *s = NULL, *p = NULL, *urlend = NULL;
	size_t limit = sizeof(httpd->recvbuf) - httpd->recvlen - 1;
	size_t len = region->length;
	int delim;
	bool truncated = false;

	if (len > limit) {
		len = limit;
		truncated = true;
	}

	if (len > 0U) {
		if (httpd->truncated) {
			return (ISC_R_NOSPACE);
		}
		memmove(httpd->recvbuf + httpd->recvlen, region->base, len);
		httpd->recvlen += len;
		httpd->recvbuf[httpd->recvlen] = 0;
		isc_region_consume(region, len);
	}
	if (truncated) {
		httpd->truncated = true;
	}
	httpd->headers = NULL;
	*buflen = httpd->recvlen;

	/*
	 * If we don't find a blank line in our buffer, return that we need
	 * more data.
	 */
	s = strstr(httpd->recvbuf, "\r\n\r\n");
	delim = 2;
	if (s == NULL) {
		s = strstr(httpd->recvbuf, "\n\n");
		delim = 1;
		if (s == NULL) {
			return (httpd->truncated ? ISC_R_NOSPACE
						 : ISC_R_NOTFOUND);
		}
		httpd->consume = s + 2 - httpd->recvbuf;
	} else {
		httpd->consume = s + 4 - httpd->recvbuf;
	}

	/*
	 * NULL terminate the request at the blank line.
	 */
	s[delim] = 0;

	/*
	 * Determine if this is a POST or GET method.  Any other values will
	 * cause an error to be returned.
	 */
	if (strncmp(httpd->recvbuf, "GET ", 4) == 0) {
		httpd->method = METHOD_GET;
		p = httpd->recvbuf + 4;
	} else if (strncmp(httpd->recvbuf, "POST ", 5) == 0) {
		httpd->method = METHOD_POST;
		p = httpd->recvbuf + 5;
	} else {
		return (ISC_R_RANGE);
	}

	/*
	 * From now on, p is the start of our buffer.
	 */

	/*
	 * Extract the URL.
	 */
	s = p;
	while (LENGTHOK(s) && BUFLENOK(s) &&
	       (*s != '\n' && *s != '\r' && *s != '\0' && *s != ' '))
	{
		s++;
	}
	if (!LENGTHOK(s)) {
		return (ISC_R_NOTFOUND);
	}
	if (!BUFLENOK(s)) {
		return (ISC_R_NOMEMORY);
	}
	urlend = s;

	/*
	 * Make the URL relative.
	 */
	if (strncmp(p, "http://", 7) == 0 || strncmp(p, "https://", 8) == 0) {
		/* Skip first '/' */
		while (*p != '/' && *p != 0) {
			p++;
		}
		if (*p == 0) {
			return (ISC_R_RANGE);
		}
		p++;
		/* Skip second '/' */
		while (*p != '/' && *p != 0) {
			p++;
		}
		if (*p == 0) {
			return (ISC_R_RANGE);
		}
		p++;
		/* Find third '/' */
		while (*p != '/' && *p != 0) {
			p++;
		}
		if (*p == 0) {
			p--;
			*p = '/';
		}
	}

	httpd->url = p;
	p = s + 1;
	s = p;

	/*
	 * Now, see if there is a question mark in the URL.  If so, this is
	 * part of the query string, and we will split it from the URL.
	 */
	httpd->querystring = strchr(httpd->url, '?');
	if (httpd->querystring != NULL) {
		*(httpd->querystring) = 0;
		httpd->querystring++;
	}

	/*
	 * Extract the HTTP/1.X protocol.  We will bounce on anything but
	 * HTTP/1.0 or HTTP/1.1 for now.
	 */
	while (LENGTHOK(s) && BUFLENOK(s) &&
	       (*s != '\n' && *s != '\r' && *s != '\0')) {
		s++;
	}
	if (!LENGTHOK(s)) {
		return (ISC_R_NOTFOUND);
	}
	if (!BUFLENOK(s)) {
		return (ISC_R_NOMEMORY);
	}
	/*
	 * Check that we have the expected eol delimiter.
	 */
	if (strncmp(s, delim == 1 ? "\n" : "\r\n", delim) != 0) {
		return (ISC_R_RANGE);
	}
	*s = 0;
	if ((strncmp(p, "HTTP/1.0", 8) != 0) &&
	    (strncmp(p, "HTTP/1.1", 8) != 0)) {
		return (ISC_R_RANGE);
	}
	httpd->protocol = p;
	p = s + delim; /* skip past eol */
	s = p;

	httpd->headers = s;

	if (have_header(httpd, "Connection:", "close", ", \t\r\n", NULL)) {
		httpd->flags |= HTTPD_CLOSE;
	}

	if (have_header(httpd, "Host:", NULL, NULL, NULL)) {
		httpd->flags |= HTTPD_FOUNDHOST;
	}

	if (strncmp(httpd->protocol, "HTTP/1.0", 8) == 0) {
		if (have_header(httpd, "Connection:", "Keep-Alive", ", \t\r\n",
				NULL)) {
			httpd->flags |= HTTPD_KEEPALIVE;
		} else {
			httpd->flags |= HTTPD_CLOSE;
		}
	}

	/*
	 * Check for Accept-Encoding:
	 */
#ifdef HAVE_ZLIB
	if (have_header(httpd, "Accept-Encoding:", "deflate", ";, \t\r\n",
			NULL)) {
		httpd->flags |= HTTPD_ACCEPT_DEFLATE;
	}
#endif /* ifdef HAVE_ZLIB */

	/*
	 * Standards compliance hooks here.
	 */
	if (strcmp(httpd->protocol, "HTTP/1.1") == 0 &&
	    ((httpd->flags & HTTPD_FOUNDHOST) == 0))
	{
		return (ISC_R_RANGE);
	}

	/*
	 * Looks like a a valid request, so now we know we won't have
	 * to process this buffer again. We can NULL-terminate the
	 * URL for the caller's benefit, and set recvlen to 0 so
	 * the next read will overwrite this one instead of appending
	 * to the buffer.
	 */
	*urlend = 0;

	return (ISC_R_SUCCESS);
}