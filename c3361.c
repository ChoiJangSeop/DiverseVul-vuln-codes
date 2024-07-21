authreadkeys(
	const char *file
	)
{
	FILE	*fp;
	char	*line;
	char	*token;
	keyid_t	keyno;
	int	keytype;
	char	buf[512];		/* lots of room for line */
	u_char	keystr[32];		/* Bug 2537 */
	size_t	len;
	size_t	j;
	u_int   nerr;
	KeyDataT *list = NULL;
	KeyDataT *next = NULL;
	/*
	 * Open file.  Complain and return if it can't be opened.
	 */
	fp = fopen(file, "r");
	if (fp == NULL) {
		msyslog(LOG_ERR, "authreadkeys: file '%s': %m",
		    file);
		goto onerror;
	}
	INIT_SSL();

	/*
	 * Now read lines from the file, looking for key entries. Put
	 * the data into temporary store for later propagation to avoid
	 * two-pass processing.
	 */
	nerr = 0;
	while ((line = fgets(buf, sizeof buf, fp)) != NULL) {
		if (nerr > nerr_maxlimit)
			break;
		token = nexttok(&line);
		if (token == NULL)
			continue;
		
		/*
		 * First is key number.  See if it is okay.
		 */
		keyno = atoi(token);
		if (keyno == 0) {
			log_maybe(&nerr,
				  "authreadkeys: cannot change key %s",
				  token);
			continue;
		}

		if (keyno > NTP_MAXKEY) {
			log_maybe(&nerr,
				  "authreadkeys: key %s > %d reserved for Autokey",
				  token, NTP_MAXKEY);
			continue;
		}

		/*
		 * Next is keytype. See if that is all right.
		 */
		token = nexttok(&line);
		if (token == NULL) {
			log_maybe(&nerr,
				  "authreadkeys: no key type for key %d",
				  keyno);
			continue;
		}
#ifdef OPENSSL
		/*
		 * The key type is the NID used by the message digest 
		 * algorithm. There are a number of inconsistencies in
		 * the OpenSSL database. We attempt to discover them
		 * here and prevent use of inconsistent data later.
		 */
		keytype = keytype_from_text(token, NULL);
		if (keytype == 0) {
			log_maybe(&nerr,
				  "authreadkeys: invalid type for key %d",
				  keyno);
			continue;
		}
		if (EVP_get_digestbynid(keytype) == NULL) {
			log_maybe(&nerr,
				  "authreadkeys: no algorithm for key %d",
				  keyno);
			continue;
		}
#else	/* !OPENSSL follows */

		/*
		 * The key type is unused, but is required to be 'M' or
		 * 'm' for compatibility.
		 */
		if (!(*token == 'M' || *token == 'm')) {
			log_maybe(&nerr,
				  "authreadkeys: invalid type for key %d",
				  keyno);
			continue;
		}
		keytype = KEY_TYPE_MD5;
#endif	/* !OPENSSL */

		/*
		 * Finally, get key and insert it. If it is longer than 20
		 * characters, it is a binary string encoded in hex;
		 * otherwise, it is a text string of printable ASCII
		 * characters.
		 */
		token = nexttok(&line);
		if (token == NULL) {
			log_maybe(&nerr,
				  "authreadkeys: no key for key %d", keyno);
			continue;
		}
		next = NULL;
		len = strlen(token);
		if (len <= 20) {	/* Bug 2537 */
			next = emalloc(sizeof(KeyDataT) + len);
			next->keyid   = keyno;
			next->keytype = keytype;
			next->seclen  = len;
			memcpy(next->secbuf, token, len);
		} else {
			static const char hex[] = "0123456789abcdef";
			u_char	temp;
			char	*ptr;
			size_t	jlim;

			jlim = min(len, 2 * sizeof(keystr));
			for (j = 0; j < jlim; j++) {
				ptr = strchr(hex, tolower((unsigned char)token[j]));
				if (ptr == NULL)
					break;	/* abort decoding */
				temp = (u_char)(ptr - hex);
				if (j & 1)
					keystr[j / 2] |= temp;
				else
					keystr[j / 2] = temp << 4;
			}
			if (j < jlim) {
				log_maybe(&nerr,
					  "authreadkeys: invalid hex digit for key %d",
					  keyno);
				continue;
			}
			len = jlim/2; /* hmmmm.... what about odd length?!? */
			next = emalloc(sizeof(KeyDataT) + len);
			next->keyid   = keyno;
			next->keytype = keytype;
			next->seclen  = len;
			memcpy(next->secbuf, keystr, len);
		}
		INSIST(NULL != next);
		next->next = list;
		list = next;
	}
	fclose(fp);
	if (nerr > nerr_maxlimit) {
		msyslog(LOG_ERR,
			"authreadkeys: rejecting file '%s' after %u errors (emergency break)",
			file, nerr);
		goto onerror;
	}
	if (nerr > 0) {
		msyslog(LOG_ERR,
			"authreadkeys: rejecting file '%s' after %u error(s)",
			file, nerr);
		goto onerror;
	}

	/* first remove old file-based keys */
	auth_delkeys();
	/* insert the new key material */
	while (NULL != (next = list)) {
		list = next->next;
		MD5auth_setkey(next->keyid, next->keytype,
			       next->secbuf, next->seclen);
		/* purge secrets from memory before free()ing it */
		memset(next, 0, sizeof(*next) + next->seclen);
		free(next);
	}
	return (1);

  onerror:
	/* Mop up temporary storage before bailing out. */
	while (NULL != (next = list)) {
		list = next->next;
		/* purge secrets from memory before free()ing it */
		memset(next, 0, sizeof(*next) + next->seclen);
		free(next);
	}
	return (0);
}