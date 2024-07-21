static char *r_socket_http_answer (RSocket *s, int *code, int *rlen) {
	r_return_val_if_fail (s, NULL);
	const char *p;
	int ret, len = 0, bufsz = 32768, delta = 0;
	char *dn, *buf = calloc (1, bufsz + 32); // XXX: use r_buffer here
	if (!buf) {
		return NULL;
	}
	char *res = NULL;
	int olen = __socket_slurp (s, (ut8*)buf, bufsz);
	if ((dn = (char*)r_str_casestr (buf, "\n\n"))) {
		delta += 2;
	} else if ((dn = (char*)r_str_casestr (buf, "\r\n\r\n"))) {
		delta += 4;
	} else {
		goto fail;
	}

	olen -= delta;
	*dn = 0; // chop headers
	/* Parse Len */
	p = r_str_casestr (buf, "Content-Length: ");
	if (p) {
		len = atoi (p + 16);
	} else {
		len = olen - (dn - buf);
	}
	if (len > 0) {
		if (len > olen) {
			res = malloc (len + 2);
			memcpy (res, dn + delta, olen);
			do {
				ret = r_socket_read_block (s, (ut8*) res + olen, len - olen);
				if (ret < 1) {
					break;
				}
				olen += ret;
			} while (olen < len);
			res[len] = 0;
		} else {
			res = malloc (len + 1);
			if (res) {
				memcpy (res, dn + delta, len);
				res[len] = 0;
			}
		}
	} else {
		res = NULL;
	}
fail:
	free (buf);
// is 's' free'd? isn't this going to cause a double free?
	r_socket_close (s);
	if (rlen) {
		*rlen = len;
	}
	return res;
}