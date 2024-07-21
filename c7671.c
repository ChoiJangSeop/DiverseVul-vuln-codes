xsltNumberFormatTokenize(const xmlChar *format,
			 xsltFormatPtr tokens)
{
    int ix = 0;
    int j;
    int val;
    int len;

    default_token.token = DEFAULT_TOKEN;
    default_token.width = 1;
    default_token.separator = BAD_CAST(DEFAULT_SEPARATOR);


    tokens->start = NULL;
    tokens->tokens[0].separator = NULL;
    tokens->end = NULL;

    /*
     * Insert initial non-alphanumeric token.
     * There is always such a token in the list, even if NULL
     */
    while (! (IS_LETTER(val=xmlStringCurrentChar(NULL, format+ix, &len)) ||
	      IS_DIGIT(val)) ) {
	if (format[ix] == 0)		/* if end of format string */
	    break; /* while */
	ix += len;
    }
    if (ix > 0)
	tokens->start = xmlStrndup(format, ix);


    for (tokens->nTokens = 0; tokens->nTokens < MAX_TOKENS;
	 tokens->nTokens++) {
	if (format[ix] == 0)
	    break; /* for */

	/*
	 * separator has already been parsed (except for the first
	 * number) in tokens->end, recover it.
	 */
	if (tokens->nTokens > 0) {
	    tokens->tokens[tokens->nTokens].separator = tokens->end;
	    tokens->end = NULL;
	}

	val = xmlStringCurrentChar(NULL, format+ix, &len);
	if (IS_DIGIT_ONE(val) ||
		 IS_DIGIT_ZERO(val)) {
	    tokens->tokens[tokens->nTokens].width = 1;
	    while (IS_DIGIT_ZERO(val)) {
		tokens->tokens[tokens->nTokens].width++;
		ix += len;
		val = xmlStringCurrentChar(NULL, format+ix, &len);
	    }
	    if (IS_DIGIT_ONE(val)) {
		tokens->tokens[tokens->nTokens].token = val - 1;
		ix += len;
		val = xmlStringCurrentChar(NULL, format+ix, &len);
	    }
	} else if ( (val == (xmlChar)'A') ||
		    (val == (xmlChar)'a') ||
		    (val == (xmlChar)'I') ||
		    (val == (xmlChar)'i') ) {
	    tokens->tokens[tokens->nTokens].token = val;
	    ix += len;
	    val = xmlStringCurrentChar(NULL, format+ix, &len);
	} else {
	    /* XSLT section 7.7
	     * "Any other format token indicates a numbering sequence
	     *  that starts with that token. If an implementation does
	     *  not support a numbering sequence that starts with that
	     *  token, it must use a format token of 1."
	     */
	    tokens->tokens[tokens->nTokens].token = (xmlChar)'0';
	    tokens->tokens[tokens->nTokens].width = 1;
	}
	/*
	 * Skip over remaining alphanumeric characters from the Nd
	 * (Number, decimal digit), Nl (Number, letter), No (Number,
	 * other), Lu (Letter, uppercase), Ll (Letter, lowercase), Lt
	 * (Letters, titlecase), Lm (Letters, modifiers), and Lo
	 * (Letters, other (uncased)) Unicode categories. This happens
	 * to correspond to the Letter and Digit classes from XML (and
	 * one wonders why XSLT doesn't refer to these instead).
	 */
	while (IS_LETTER(val) || IS_DIGIT(val)) {
	    ix += len;
	    val = xmlStringCurrentChar(NULL, format+ix, &len);
	}

	/*
	 * Insert temporary non-alphanumeric final tooken.
	 */
	j = ix;
	while (! (IS_LETTER(val) || IS_DIGIT(val))) {
	    if (val == 0)
		break; /* while */
	    ix += len;
	    val = xmlStringCurrentChar(NULL, format+ix, &len);
	}
	if (ix > j)
	    tokens->end = xmlStrndup(&format[j], ix - j);
    }
}