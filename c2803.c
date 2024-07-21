ftp_pasv(ftpbuf_t *ftp, int pasv)
{
	char			*ptr;
	union ipbox		ipbox;
	unsigned long		b[6];
	socklen_t			n;
	struct sockaddr *sa;
	struct sockaddr_in *sin;

	if (ftp == NULL) {
		return 0;
	}
	if (pasv && ftp->pasv == 2) {
		return 1;
	}
	ftp->pasv = 0;
	if (!pasv) {
		return 1;
	}
	n = sizeof(ftp->pasvaddr);
	memset(&ftp->pasvaddr, 0, n);
	sa = (struct sockaddr *) &ftp->pasvaddr;

#if HAVE_IPV6
	if (getpeername(ftp->fd, sa, &n) < 0) {
		return 0;
	}
	if (sa->sa_family == AF_INET6) {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) sa;
		char *endptr, delimiter;

		/* try EPSV first */
		if (!ftp_putcmd(ftp, "EPSV", NULL)) {
			return 0;
		}
		if (!ftp_getresp(ftp)) {
			return 0;
		}
		if (ftp->resp == 229) {
			/* parse out the port */
			for (ptr = ftp->inbuf; *ptr && *ptr != '('; ptr++);
			if (!*ptr) {
				return 0;
			}
			delimiter = *++ptr;
			for (n = 0; *ptr && n < 3; ptr++) {
				if (*ptr == delimiter) {
					n++;
				}
			}

			sin6->sin6_port = htons((unsigned short) strtoul(ptr, &endptr, 10));
			if (ptr == endptr || *endptr != delimiter) {
				return 0;
			}
			ftp->pasv = 2;
			return 1;
		}
	}

	/* fall back to PASV */
#endif

	if (!ftp_putcmd(ftp, "PASV", NULL)) {
		return 0;
	}
	if (!ftp_getresp(ftp) || ftp->resp != 227) { 
		return 0;
	}
	/* parse out the IP and port */
	for (ptr = ftp->inbuf; *ptr && !isdigit(*ptr); ptr++);
	n = sscanf(ptr, "%lu,%lu,%lu,%lu,%lu,%lu", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]);
	if (n != 6) {
		return 0;
	}
	for (n = 0; n < 6; n++) {
		ipbox.c[n] = (unsigned char) b[n];
	}
	sin = (struct sockaddr_in *) sa;
	sin->sin_family = AF_INET;
	sin->sin_addr = ipbox.ia[0];
	sin->sin_port = ipbox.s[2];

	ftp->pasv = 2;

	return 1;
}