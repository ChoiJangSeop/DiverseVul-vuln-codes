smtp_mailaddr(struct mailaddr *maddr, char *line, int mailfrom, char **args,
    const char *domain)
{
	char   *p, *e;

	if (line == NULL)
		return (0);

	if (*line != '<')
		return (0);

	e = strchr(line, '>');
	if (e == NULL)
		return (0);
	*e++ = '\0';
	while (*e == ' ')
		e++;
	*args = e;

	if (!text_to_mailaddr(maddr, line + 1))
		return (0);

	p = strchr(maddr->user, ':');
	if (p != NULL) {
		p++;
		memmove(maddr->user, p, strlen(p) + 1);
	}

	if (!valid_localpart(maddr->user) ||
	    !valid_domainpart(maddr->domain)) {
		/* accept empty return-path in MAIL FROM, required for bounces */
		if (mailfrom && maddr->user[0] == '\0' && maddr->domain[0] == '\0')
			return (1);

		/* no user-part, reject */
		if (maddr->user[0] == '\0')
			return (0);

		/* no domain, local user */
		if (maddr->domain[0] == '\0') {
			(void)strlcpy(maddr->domain, domain,
			    sizeof(maddr->domain));
			return (1);
		}
		return (0);
	}

	return (1);
}