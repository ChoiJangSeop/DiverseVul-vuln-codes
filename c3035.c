PHPAPI pcre_cache_entry* pcre_get_compiled_regex_cache(char *regex, int regex_len TSRMLS_DC)
{
	pcre				*re = NULL;
	pcre_extra			*extra;
	int					 coptions = 0;
	int					 soptions = 0;
	const char			*error;
	int					 erroffset;
	char				 delimiter;
	char				 start_delimiter;
	char				 end_delimiter;
	char				*p, *pp;
	char				*pattern;
	int					 do_study = 0;
	int					 poptions = 0;
	int				count = 0;
	unsigned const char *tables = NULL;
#if HAVE_SETLOCALE
	char				*locale;
#endif
	pcre_cache_entry	*pce;
	pcre_cache_entry	 new_entry;
	char                *tmp = NULL;

#if HAVE_SETLOCALE
# if defined(PHP_WIN32) && defined(ZTS)
	_configthreadlocale(_ENABLE_PER_THREAD_LOCALE);
# endif
	locale = setlocale(LC_CTYPE, NULL);
#endif

	/* Try to lookup the cached regex entry, and if successful, just pass
	   back the compiled pattern, otherwise go on and compile it. */
	if (zend_hash_find(&PCRE_G(pcre_cache), regex, regex_len+1, (void **)&pce) == SUCCESS) {
		/*
		 * We use a quick pcre_fullinfo() check to see whether cache is corrupted, and if it
		 * is, we flush it and compile the pattern from scratch.
		 */
		if (pcre_fullinfo(pce->re, NULL, PCRE_INFO_CAPTURECOUNT, &count) == PCRE_ERROR_BADMAGIC) {
			zend_hash_clean(&PCRE_G(pcre_cache));
		} else {
#if HAVE_SETLOCALE
			if (!strcmp(pce->locale, locale)) {
#endif
				return pce;
#if HAVE_SETLOCALE
			}
#endif
		}
	}
	
	p = regex;
	
	/* Parse through the leading whitespace, and display a warning if we
	   get to the end without encountering a delimiter. */
	while (isspace((int)*(unsigned char *)p)) p++;
	if (*p == 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, 
						 p < regex + regex_len ? "Null byte in regex" : "Empty regular expression");
		return NULL;
	}
	
	/* Get the delimiter and display a warning if it is alphanumeric
	   or a backslash. */
	delimiter = *p++;
	if (isalnum((int)*(unsigned char *)&delimiter) || delimiter == '\\') {
		php_error_docref(NULL TSRMLS_CC,E_WARNING, "Delimiter must not be alphanumeric or backslash");
		return NULL;
	}

	start_delimiter = delimiter;
	if ((pp = strchr("([{< )]}> )]}>", delimiter)))
		delimiter = pp[5];
	end_delimiter = delimiter;

	pp = p;

	if (start_delimiter == end_delimiter) {
		/* We need to iterate through the pattern, searching for the ending delimiter,
		   but skipping the backslashed delimiters.  If the ending delimiter is not
		   found, display a warning. */
		while (*pp != 0) {
			if (*pp == '\\' && pp[1] != 0) pp++;
			else if (*pp == delimiter)
				break;
			pp++;
		}
	} else {
		/* We iterate through the pattern, searching for the matching ending
		 * delimiter. For each matching starting delimiter, we increment nesting
		 * level, and decrement it for each matching ending delimiter. If we
		 * reach the end of the pattern without matching, display a warning.
		 */
		int brackets = 1; 	/* brackets nesting level */
		while (*pp != 0) {
			if (*pp == '\\' && pp[1] != 0) pp++;
			else if (*pp == end_delimiter && --brackets <= 0)
				break;
			else if (*pp == start_delimiter)
				brackets++;
			pp++;
		}
	}

	if (*pp == 0) {
		if (pp < regex + regex_len) {
			php_error_docref(NULL TSRMLS_CC,E_WARNING, "Null byte in regex");
		} else if (start_delimiter == end_delimiter) {
			php_error_docref(NULL TSRMLS_CC,E_WARNING, "No ending delimiter '%c' found", delimiter);
		} else {
			php_error_docref(NULL TSRMLS_CC,E_WARNING, "No ending matching delimiter '%c' found", delimiter);
		}
		return NULL;
	}
	
	/* Make a copy of the actual pattern. */
	pattern = estrndup(p, pp-p);

	/* Move on to the options */
	pp++;

	/* Parse through the options, setting appropriate flags.  Display
	   a warning if we encounter an unknown modifier. */	
	while (pp < regex + regex_len) {
		switch (*pp++) {
			/* Perl compatible options */
			case 'i':	coptions |= PCRE_CASELESS;		break;
			case 'm':	coptions |= PCRE_MULTILINE;		break;
			case 's':	coptions |= PCRE_DOTALL;		break;
			case 'x':	coptions |= PCRE_EXTENDED;		break;
			
			/* PCRE specific options */
			case 'A':	coptions |= PCRE_ANCHORED;		break;
			case 'D':	coptions |= PCRE_DOLLAR_ENDONLY;break;
			case 'S':	do_study  = 1;					break;
			case 'U':	coptions |= PCRE_UNGREEDY;		break;
			case 'X':	coptions |= PCRE_EXTRA;			break;
			case 'u':	coptions |= PCRE_UTF8;
	/* In  PCRE,  by  default, \d, \D, \s, \S, \w, and \W recognize only ASCII
       characters, even in UTF-8 mode. However, this can be changed by setting
       the PCRE_UCP option. */
#ifdef PCRE_UCP
						coptions |= PCRE_UCP;
#endif			
				break;

			/* Custom preg options */
			case 'e':	poptions |= PREG_REPLACE_EVAL;	break;
			
			case ' ':
			case '\n':
				break;

			default:
				if (pp[-1]) {
					php_error_docref(NULL TSRMLS_CC,E_WARNING, "Unknown modifier '%c'", pp[-1]);
				} else {
					php_error_docref(NULL TSRMLS_CC,E_WARNING, "Null byte in regex");
				}
				efree(pattern);
				return NULL;
		}
	}

#if HAVE_SETLOCALE
	if (strcmp(locale, "C"))
		tables = pcre_maketables();
#endif

	/* Compile pattern and display a warning if compilation failed. */
	re = pcre_compile(pattern,
					  coptions,
					  &error,
					  &erroffset,
					  tables);

	if (re == NULL) {
		php_error_docref(NULL TSRMLS_CC,E_WARNING, "Compilation failed: %s at offset %d", error, erroffset);
		efree(pattern);
		if (tables) {
			pefree((void*)tables, 1);
		}
		return NULL;
	}

	/* If study option was specified, study the pattern and
	   store the result in extra for passing to pcre_exec. */
	if (do_study) {
		extra = pcre_study(re, soptions, &error);
		if (extra) {
			extra->flags |= PCRE_EXTRA_MATCH_LIMIT | PCRE_EXTRA_MATCH_LIMIT_RECURSION;
		}
		if (error != NULL) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error while studying pattern");
		}
	} else {
		extra = NULL;
	}

	efree(pattern);

	/*
	 * If we reached cache limit, clean out the items from the head of the list;
	 * these are supposedly the oldest ones (but not necessarily the least used
	 * ones).
	 */
	if (zend_hash_num_elements(&PCRE_G(pcre_cache)) == PCRE_CACHE_SIZE) {
		int num_clean = PCRE_CACHE_SIZE / 8;
		zend_hash_apply_with_argument(&PCRE_G(pcre_cache), pcre_clean_cache, &num_clean TSRMLS_CC);
	}

	/* Store the compiled pattern and extra info in the cache. */
	new_entry.re = re;
	new_entry.extra = extra;
	new_entry.preg_options = poptions;
	new_entry.compile_options = coptions;
#if HAVE_SETLOCALE
	new_entry.locale = pestrdup(locale, 1);
	new_entry.tables = tables;
#endif

	/*
	 * Interned strings are not duplicated when stored in HashTable,
	 * but all the interned strings created during HTTP request are removed
	 * at end of request. However PCRE_G(pcre_cache) must be consistent
	 * on the next request as well. So we disable usage of interned strings
	 * as hash keys especually for this table.
	 * See bug #63180 
	 */
	if (IS_INTERNED(regex)) {
		regex = tmp = estrndup(regex, regex_len);
	}

	zend_hash_update(&PCRE_G(pcre_cache), regex, regex_len+1, (void *)&new_entry,
						sizeof(pcre_cache_entry), (void**)&pce);

	if (tmp) {
		efree(tmp);
	}

	return pce;
}