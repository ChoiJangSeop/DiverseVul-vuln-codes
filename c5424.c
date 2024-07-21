messageFindArgument(const message *m, const char *variable)
{
	int i;
	size_t len;

	assert(m != NULL);
	assert(variable != NULL);

	len = strlen(variable);

	for(i = 0; i < m->numberOfArguments; i++) {
		const char *ptr;

		ptr = messageGetArgument(m, i);
		if((ptr == NULL) || (*ptr == '\0'))
			continue;
#ifdef	CL_DEBUG
		cli_dbgmsg("messageFindArgument: compare %lu bytes of %s with %s\n",
			(unsigned long)len, variable, ptr);
#endif
		if(strncasecmp(ptr, variable, len) == 0) {
			ptr = &ptr[len];
			while(isspace(*ptr))
				ptr++;
			if(*ptr != '=') {
				cli_dbgmsg("messageFindArgument: no '=' sign found in MIME header '%s' (%s)\n", variable, messageGetArgument(m, i));
				return NULL;
			}
			if((*++ptr == '"') && (strchr(&ptr[1], '"') != NULL)) {
				/* Remove any quote characters */
				char *ret = cli_strdup(++ptr);
				char *p;

				if(ret == NULL)
					return NULL;

				/*
				 * fix un-quoting of boundary strings from
				 * header, occurs if boundary was given as
				 *	'boundary="_Test_";'
				 *
				 * At least two quotes in string, assume
				 * quoted argument
				 * end string at next quote
				 */
				if((p = strchr(ret, '"')) != NULL) {
					ret[strlen(ret) - 1] = '\0';
					*p = '\0';
				}
				return ret;
			}
			return cli_strdup(ptr);
		}
	}
	return NULL;
}