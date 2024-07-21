xsltNumberFormatInsertNumbers(xsltNumberDataPtr data,
			      double *numbers,
			      int numbers_max,
			      xsltFormatPtr tokens,
			      xmlBufferPtr buffer)
{
    int i = 0;
    double number;
    xsltFormatTokenPtr token;

    /*
     * Handle initial non-alphanumeric token
     */
    if (tokens->start != NULL)
	 xmlBufferCat(buffer, tokens->start);

    for (i = 0; i < numbers_max; i++) {
	/* Insert number */
	number = numbers[(numbers_max - 1) - i];
        /* Round to nearest like XSLT 2.0 */
        number = floor(number + 0.5);
        /*
         * XSLT 1.0 isn't clear on how to handle negative numbers, but XSLT
         * 2.0 says:
         *
         *     It is a non-recoverable dynamic error if any undiscarded item
         *     in the atomized sequence supplied as the value of the value
         *     attribute of xsl:number cannot be converted to an integer, or
         *     if the resulting integer is less than 0 (zero).
         */
        if (number < 0.0) {
            xsltTransformError(NULL, NULL, NULL,
                    "xsl-number : negative value\n");
            /* Recover by treating negative values as zero. */
            number = 0.0;
        }
	if (i < tokens->nTokens) {
	  /*
	   * The "n"th format token will be used to format the "n"th
	   * number in the list
	   */
	  token = &(tokens->tokens[i]);
	} else if (tokens->nTokens > 0) {
	  /*
	   * If there are more numbers than format tokens, then the
	   * last format token will be used to format the remaining
	   * numbers.
	   */
	  token = &(tokens->tokens[tokens->nTokens - 1]);
	} else {
	  /*
	   * If there are no format tokens, then a format token of
	   * 1 is used to format all numbers.
	   */
	  token = &default_token;
	}

	/* Print separator, except for the first number */
	if (i > 0) {
	    if (token->separator != NULL)
		xmlBufferCat(buffer, token->separator);
	    else
		xmlBufferCCat(buffer, DEFAULT_SEPARATOR);
	}

	switch (xmlXPathIsInf(number)) {
	case -1:
	    xmlBufferCCat(buffer, "-Infinity");
	    break;
	case 1:
	    xmlBufferCCat(buffer, "Infinity");
	    break;
	default:
	    if (xmlXPathIsNaN(number)) {
		xmlBufferCCat(buffer, "NaN");
	    } else {

		switch (token->token) {
		case 'A':
		    xsltNumberFormatAlpha(data, buffer, number, TRUE);
		    break;
		case 'a':
		    xsltNumberFormatAlpha(data, buffer, number, FALSE);
		    break;
		case 'I':
		    xsltNumberFormatRoman(buffer,
					  number,
					  TRUE);

		    break;
		case 'i':
		    xsltNumberFormatRoman(buffer,
					  number,
					  FALSE);

		    break;
		default:
		    if (IS_DIGIT_ZERO(token->token)) {
			xsltNumberFormatDecimal(buffer,
						number,
						token->token,
						token->width,
						data->digitsPerGroup,
						data->groupingCharacter,
						data->groupingCharacterLen);
		    }
		    break;
		}
	    }

	}
    }

    /*
     * Handle final non-alphanumeric token
     */
    if (tokens->end != NULL)
	 xmlBufferCat(buffer, tokens->end);

}