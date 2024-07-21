con_header_read(agooCon c, size_t *mlenp) {
    char		*hend = strstr(c->buf, "\r\n\r\n");
    agooMethod		method;
    struct _agooSeg	path;
    char		*query = NULL;
    char		*qend;
    char		*b;
    size_t		clen = 0;
    long		mlen;
    agooHook		hook = NULL;
    agooPage		p;
    struct _agooErr	err = AGOO_ERR_INIT;

    if (NULL == hend) {
	if (sizeof(c->buf) - 1 <= c->bcnt) {
	    return bad_request(c, 431, __LINE__);
	}
	return HEAD_AGAIN;
    }
    if (agoo_req_cat.on) {
	*hend = '\0';
	agoo_log_cat(&agoo_req_cat, "%s %llu: %s", agoo_con_kind_str(c->bind->kind), (unsigned long long)c->id, c->buf);
	*hend = '\r';
    }
    for (b = c->buf; ' ' != *b; b++) {
	if ('\0' == *b) {
	    return bad_request(c, 400, __LINE__);
	}
    }
    switch (toupper(*c->buf)) {
    case 'G':
	if (3 != b - c->buf || 0 != strncmp("GET", c->buf, 3)) {
	    return bad_request(c, 400, __LINE__);
	}
	method = AGOO_GET;
	break;
    case 'P': {
	const char	*v;
	int		vlen = 0;
	char		*vend;

	if (3 == b - c->buf && 0 == strncmp("PUT", c->buf, 3)) {
	    method = AGOO_PUT;
	} else if (4 == b - c->buf && 0 == strncmp("POST", c->buf, 4)) {
	    method = AGOO_POST;
	} else {
	    return bad_request(c, 400, __LINE__);
	}
	if (NULL == (v = agoo_con_header_value(c->buf, (int)(hend - c->buf), "Content-Length", &vlen))) {
	    return bad_request(c, 411, __LINE__);
	}
	clen = (size_t)strtoul(v, &vend, 10);
	if (vend != v + vlen) {
	    return bad_request(c, 411, __LINE__);
	}
	break;
    }
    case 'D':
	if (6 != b - c->buf || 0 != strncmp("DELETE", c->buf, 6)) {
	    return bad_request(c, 400, __LINE__);
	}
	method = AGOO_DELETE;
	break;
    case 'H':
	if (4 != b - c->buf || 0 != strncmp("HEAD", c->buf, 4)) {
	    return bad_request(c, 400, __LINE__);
	}
	method = AGOO_HEAD;
	break;
    case 'O':
	if (7 != b - c->buf || 0 != strncmp("OPTIONS", c->buf, 7)) {
	    return bad_request(c, 400, __LINE__);
	}
	method = AGOO_OPTIONS;
	break;
    case 'C':
	if (7 != b - c->buf || 0 != strncmp("CONNECT", c->buf, 7)) {
	    return bad_request(c, 400, __LINE__);
	}
	method = AGOO_CONNECT;
	break;
    default:
	return bad_request(c, 400, __LINE__);
    }
    for (; ' ' == *b; b++) {
	if ('\0' == *b) {
	    return bad_request(c, 400, __LINE__);
	}
    }
    path.start = b;
    for (; ' ' != *b; b++) {
	switch (*b) {
	case '?':
	    path.end = b;
	    query = b + 1;
	    break;
	case '\0':
	    return bad_request(c, 400, __LINE__);
	default:
	    break;
	}
    }
    if (NULL == query) {
	path.end = b;
	query = b;
	qend = b;
    } else {
	qend = b;
    }
    mlen = hend - c->buf + 4 + clen;
    *mlenp = mlen;

    if (AGOO_GET == method) {
	char		root_buf[20148];
	const char	*root = NULL;

	if (NULL != (p = agoo_group_get(&err, path.start, (int)(path.end - path.start)))) {
	    if (page_response(c, p, hend)) {
		return bad_request(c, 500, __LINE__);
	    }
	    return HEAD_HANDLED;
	}
	if (agoo_domain_use()) {
	    const char	*host;
	    int		vlen = 0;

	    if (NULL == (host = agoo_con_header_value(c->buf, (int)(hend - c->buf), "Host", &vlen))) {
		return bad_request(c, 411, __LINE__);
	    }
	    ((char*)host)[vlen] = '\0';
	    root = agoo_domain_resolve(host, root_buf, sizeof(root_buf));
	    ((char*)host)[vlen] = '\r';
	}
	if (agoo_server.root_first &&
	    NULL != (p = agoo_page_get(&err, path.start, (int)(path.end - path.start), root))) {
	    if (page_response(c, p, hend)) {
		return bad_request(c, 500, __LINE__);
	    }
	    return HEAD_HANDLED;
	}
	if (NULL == (hook = agoo_hook_find(agoo_server.hooks, method, &path))) {
	    if (NULL != (p = agoo_page_get(&err, path.start, (int)(path.end - path.start), root))) {
		if (page_response(c, p, hend)) {
		    return bad_request(c, 500, __LINE__);
		}
		return HEAD_HANDLED;
	    }
	    if (NULL == agoo_server.hook404) {
		return bad_request(c, 404, __LINE__);
	    }
	    hook = agoo_server.hook404;
	}
    } else if (NULL == (hook = agoo_hook_find(agoo_server.hooks, method, &path))) {
 	return bad_request(c, 404, __LINE__);
    }
    // Create request and populate.
    if (NULL == (c->req = agoo_req_create(mlen))) {
	return bad_request(c, 413, __LINE__);
    }
    if ((long)c->bcnt <= mlen) {
	memcpy(c->req->msg, c->buf, c->bcnt);
	if ((long)c->bcnt < mlen) {
	    memset(c->req->msg + c->bcnt, 0, mlen - c->bcnt);
	}
    } else {
	memcpy(c->req->msg, c->buf, mlen);
    }
    c->req->msg[mlen] = '\0';
    c->req->method = method;
    c->req->upgrade = AGOO_UP_NONE;
    c->req->up = NULL;
    c->req->path.start = c->req->msg + (path.start - c->buf);
    c->req->path.len = (int)(path.end - path.start);
    c->req->query.start = c->req->msg + (query - c->buf);
    c->req->query.len = (int)(qend - query);
    c->req->query.start[c->req->query.len] = '\0';
    c->req->body.start = c->req->msg + (hend - c->buf + 4);
    c->req->body.len = (unsigned int)clen;
    b = strstr(b, "\r\n");
    c->req->header.start = c->req->msg + (b + 2 - c->buf);
    if (b < hend) {
	c->req->header.len = (unsigned int)(hend - b - 2);
    } else {
	c->req->header.len = 0;
    }
    c->req->res = NULL;
    c->req->hook = hook;

    return HEAD_OK;
}