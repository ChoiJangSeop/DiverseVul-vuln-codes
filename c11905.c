static void parse_extra_args(char *extra_args, int buflen)
{
	char *val;
	int vallen;
	char *end = extra_args + buflen;

	while (extra_args < end && *extra_args) {
		saw_extended_args = 1;
		if (strncasecmp("host=", extra_args, 5) == 0) {
			val = extra_args + 5;
			vallen = strlen(val) + 1;
			if (*val) {
				/* Split <host>:<port> at colon. */
				char *host = val;
				char *port = strrchr(host, ':');
				if (port) {
					*port = 0;
					port++;
					free(tcp_port);
					tcp_port = xstrdup(port);
				}
				free(hostname);
				hostname = xstrdup_tolower(host);
			}

			/* On to the next one */
			extra_args = val + vallen;
		}
	}

	/*
	 * Locate canonical hostname and its IP address.
	 */
	if (hostname) {
#ifndef NO_IPV6
		struct addrinfo hints;
		struct addrinfo *ai;
		int gai;
		static char addrbuf[HOST_NAME_MAX + 1];

		memset(&hints, 0, sizeof(hints));
		hints.ai_flags = AI_CANONNAME;

		gai = getaddrinfo(hostname, 0, &hints, &ai);
		if (!gai) {
			struct sockaddr_in *sin_addr = (void *)ai->ai_addr;

			inet_ntop(AF_INET, &sin_addr->sin_addr,
				  addrbuf, sizeof(addrbuf));
			free(ip_address);
			ip_address = xstrdup(addrbuf);

			free(canon_hostname);
			canon_hostname = xstrdup(ai->ai_canonname ?
						 ai->ai_canonname : ip_address);

			freeaddrinfo(ai);
		}
#else
		struct hostent *hent;
		struct sockaddr_in sa;
		char **ap;
		static char addrbuf[HOST_NAME_MAX + 1];

		hent = gethostbyname(hostname);

		ap = hent->h_addr_list;
		memset(&sa, 0, sizeof sa);
		sa.sin_family = hent->h_addrtype;
		sa.sin_port = htons(0);
		memcpy(&sa.sin_addr, *ap, hent->h_length);

		inet_ntop(hent->h_addrtype, &sa.sin_addr,
			  addrbuf, sizeof(addrbuf));

		free(canon_hostname);
		canon_hostname = xstrdup(hent->h_name);
		free(ip_address);
		ip_address = xstrdup(addrbuf);
#endif
	}
}