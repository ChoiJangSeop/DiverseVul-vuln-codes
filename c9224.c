webSocketsHandshake(rfbClientPtr cl, char *scheme)
{
    char *buf, *response, *line;
    int n, linestart = 0, len = 0, llen, base64 = TRUE;
    char prefix[5], trailer[17];
    char *path = NULL, *host = NULL, *origin = NULL, *protocol = NULL;
    char *key1 = NULL, *key2 = NULL, *key3 = NULL;
    char *sec_ws_origin = NULL;
    char *sec_ws_key = NULL;
    char sec_ws_version = 0;
    ws_ctx_t *wsctx = NULL;

    buf = (char *) malloc(WEBSOCKETS_MAX_HANDSHAKE_LEN);
    if (!buf) {
        rfbLogPerror("webSocketsHandshake: malloc");
        return FALSE;
    }
    response = (char *) malloc(WEBSOCKETS_MAX_HANDSHAKE_LEN);
    if (!response) {
        free(buf);
        rfbLogPerror("webSocketsHandshake: malloc");
        return FALSE;
    }

    while (len < WEBSOCKETS_MAX_HANDSHAKE_LEN-1) {
        if ((n = rfbReadExactTimeout(cl, buf+len, 1,
                                     WEBSOCKETS_CLIENT_SEND_WAIT_MS)) <= 0) {
            if ((n < 0) && (errno == ETIMEDOUT)) {
                break;
            }
            if (n == 0)
                rfbLog("webSocketsHandshake: client gone\n");
            else
                rfbLogPerror("webSocketsHandshake: read");
            free(response);
            free(buf);
            return FALSE;
        }

        len += 1;
        llen = len - linestart;
        if (((llen >= 2)) && (buf[len-1] == '\n')) {
            line = buf+linestart;
            if ((llen == 2) && (strncmp("\r\n", line, 2) == 0)) {
                if (key1 && key2) {
                    if ((n = rfbReadExact(cl, buf+len, 8)) <= 0) {
                        if ((n < 0) && (errno == ETIMEDOUT)) {
                            break;
                        }
                        if (n == 0)
                            rfbLog("webSocketsHandshake: client gone\n");
                        else
                            rfbLogPerror("webSocketsHandshake: read");
                        free(response);
                        free(buf);
                        return FALSE;
                    }
                    rfbLog("Got key3\n");
                    key3 = buf+len;
                    len += 8;
                } else {
                    buf[len] = '\0';
                }
                break;
            } else if ((llen >= 16) && ((strncmp("GET ", line, min(llen,4))) == 0)) {
                /* 16 = 4 ("GET ") + 1 ("/.*") + 11 (" HTTP/1.1\r\n") */
                path = line+4;
                buf[len-11] = '\0'; /* Trim trailing " HTTP/1.1\r\n" */
                cl->wspath = strdup(path);
                /* rfbLog("Got path: %s\n", path); */
            } else if ((strncasecmp("host: ", line, min(llen,6))) == 0) {
                host = line+6;
                buf[len-2] = '\0';
                /* rfbLog("Got host: %s\n", host); */
            } else if ((strncasecmp("origin: ", line, min(llen,8))) == 0) {
                origin = line+8;
                buf[len-2] = '\0';
                /* rfbLog("Got origin: %s\n", origin); */
            } else if ((strncasecmp("sec-websocket-key1: ", line, min(llen,20))) == 0) {
                key1 = line+20;
                buf[len-2] = '\0';
                /* rfbLog("Got key1: %s\n", key1); */
            } else if ((strncasecmp("sec-websocket-key2: ", line, min(llen,20))) == 0) {
                key2 = line+20;
                buf[len-2] = '\0';
                /* rfbLog("Got key2: %s\n", key2); */
            /* HyBI */

	    } else if ((strncasecmp("sec-websocket-protocol: ", line, min(llen,24))) == 0) {
                protocol = line+24;
                buf[len-2] = '\0';
                rfbLog("Got protocol: %s\n", protocol);
            } else if ((strncasecmp("sec-websocket-origin: ", line, min(llen,22))) == 0) {
		sec_ws_origin = line+22;
                buf[len-2] = '\0';
            } else if ((strncasecmp("sec-websocket-key: ", line, min(llen,19))) == 0) {
		sec_ws_key = line+19;
                buf[len-2] = '\0';
            } else if ((strncasecmp("sec-websocket-version: ", line, min(llen,23))) == 0) {
		sec_ws_version = strtol(line+23, NULL, 10);
                buf[len-2] = '\0';
	    }

            linestart = len;
        }
    }

    if (!(path && host && (origin || sec_ws_origin))) {
        rfbErr("webSocketsHandshake: incomplete client handshake\n");
        free(response);
        free(buf);
        return FALSE;
    }

    if ((protocol) && (strstr(protocol, "binary"))) {
        if (! sec_ws_version) {
            rfbErr("webSocketsHandshake: 'binary' protocol not supported with Hixie\n");
            free(response);
            free(buf);
            return FALSE;
        }
        rfbLog("  - webSocketsHandshake: using binary/raw encoding\n");
        base64 = FALSE;
        protocol = "binary";
    } else {
        rfbLog("  - webSocketsHandshake: using base64 encoding\n");
        base64 = TRUE;
        if ((protocol) && (strstr(protocol, "base64"))) {
            protocol = "base64";
        } else {
            protocol = "";
        }
    }

    /*
     * Generate the WebSockets server response based on the the headers sent
     * by the client.
     */

    if (sec_ws_version) {
	char accept[B64LEN(SHA1_HASH_SIZE) + 1];
	rfbLog("  - WebSockets client version hybi-%02d\n", sec_ws_version);
	webSocketsGenSha1Key(accept, sizeof(accept), sec_ws_key);
        if(strlen(protocol) > 0)
            len = snprintf(response, WEBSOCKETS_MAX_HANDSHAKE_LEN,
	                   SERVER_HANDSHAKE_HYBI, accept, protocol);
        else
            len = snprintf(response, WEBSOCKETS_MAX_HANDSHAKE_LEN,
                           SERVER_HANDSHAKE_HYBI_NO_PROTOCOL, accept);
    } else {
	/* older hixie handshake, this could be removed if
	 * a final standard is established */
	if (!(key1 && key2 && key3)) {
	    rfbLog("  - WebSockets client version hixie-75\n");
	    prefix[0] = '\0';
	    trailer[0] = '\0';
	} else {
	    rfbLog("  - WebSockets client version hixie-76\n");
	    snprintf(prefix, 5, "Sec-");
	    webSocketsGenMd5(trailer, key1, key2, key3);
	}
	len = snprintf(response, WEBSOCKETS_MAX_HANDSHAKE_LEN,
		 SERVER_HANDSHAKE_HIXIE, prefix, origin, prefix, scheme,
		 host, path, prefix, protocol, trailer);
    }

    if (rfbWriteExact(cl, response, len) < 0) {
        rfbErr("webSocketsHandshake: failed sending WebSockets response\n");
        free(response);
        free(buf);
        return FALSE;
    }
    /* rfbLog("webSocketsHandshake: %s\n", response); */
    free(response);
    free(buf);


    wsctx = calloc(1, sizeof(ws_ctx_t));
    if (sec_ws_version) {
	wsctx->version = WEBSOCKETS_VERSION_HYBI;
	wsctx->encode = webSocketsEncodeHybi;
	wsctx->decode = webSocketsDecodeHybi;
    } else {
	wsctx->version = WEBSOCKETS_VERSION_HIXIE;
	wsctx->encode = webSocketsEncodeHixie;
	wsctx->decode = webSocketsDecodeHixie;
    }
    wsctx->base64 = base64;
    cl->wsctx = (wsCtx *)wsctx;
    return TRUE;
}