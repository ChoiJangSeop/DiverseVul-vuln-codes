PHPAPI void php_pcre_split_impl(pcre_cache_entry *pce, char *subject, int subject_len, zval *return_value,
	long limit_val, long flags TSRMLS_DC)
{
	pcre_extra		*extra = NULL;		/* Holds results of studying */
	pcre			*re_bump = NULL;	/* Regex instance for empty matches */
	pcre_extra		*extra_bump = NULL;	/* Almost dummy */
	pcre_extra		 extra_data;		/* Used locally for exec options */
	int				*offsets;			/* Array of subpattern offsets */
	int				 size_offsets;		/* Size of the offsets array */
	int				 exoptions = 0;		/* Execution options */
	int				 count = 0;			/* Count of matched subpatterns */
	int				 start_offset;		/* Where the new search starts */
	int				 next_offset;		/* End of the last delimiter match + 1 */
	int				 g_notempty = 0;	/* If the match should not be empty */
	char			*last_match;		/* Location of last match */
	int				 rc;
	int				 no_empty;			/* If NO_EMPTY flag is set */
	int				 delim_capture; 	/* If delimiters should be captured */
	int				 offset_capture;	/* If offsets should be captured */

	no_empty = flags & PREG_SPLIT_NO_EMPTY;
	delim_capture = flags & PREG_SPLIT_DELIM_CAPTURE;
	offset_capture = flags & PREG_SPLIT_OFFSET_CAPTURE;
	
	if (limit_val == 0) {
		limit_val = -1;
	}

	if (extra == NULL) {
		extra_data.flags = PCRE_EXTRA_MATCH_LIMIT | PCRE_EXTRA_MATCH_LIMIT_RECURSION;
		extra = &extra_data;
	}
	extra->match_limit = PCRE_G(backtrack_limit);
	extra->match_limit_recursion = PCRE_G(recursion_limit);
	
	/* Initialize return value */
	array_init(return_value);

	/* Calculate the size of the offsets array, and allocate memory for it. */
	rc = pcre_fullinfo(pce->re, extra, PCRE_INFO_CAPTURECOUNT, &size_offsets);
	if (rc < 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Internal pcre_fullinfo() error %d", rc);
		RETURN_FALSE;
	}
	size_offsets = (size_offsets + 1) * 3;
	offsets = (int *)safe_emalloc(size_offsets, sizeof(int), 0);
	
	/* Start at the beginning of the string */
	start_offset = 0;
	next_offset = 0;
	last_match = subject;
	PCRE_G(error_code) = PHP_PCRE_NO_ERROR;
	
	/* Get next piece if no limit or limit not yet reached and something matched*/
	while ((limit_val == -1 || limit_val > 1)) {
		count = pcre_exec(pce->re, extra, subject,
						  subject_len, start_offset,
						  exoptions|g_notempty, offsets, size_offsets);

		/* the string was already proved to be valid UTF-8 */
		exoptions |= PCRE_NO_UTF8_CHECK;

		/* Check for too many substrings condition. */
		if (count == 0) {
			php_error_docref(NULL TSRMLS_CC,E_NOTICE, "Matched, but too many substrings");
			count = size_offsets/3;
		}
				
		/* If something matched */
		if (count > 0) {
			if (!no_empty || &subject[offsets[0]] != last_match) {

				if (offset_capture) {
					/* Add (match, offset) pair to the return value */
					add_offset_pair(return_value, last_match, &subject[offsets[0]]-last_match, next_offset, NULL);
				} else {
					/* Add the piece to the return value */
					add_next_index_stringl(return_value, last_match,
								   	   &subject[offsets[0]]-last_match, 1);
				}

				/* One less left to do */
				if (limit_val != -1)
					limit_val--;
			}
			
			last_match = &subject[offsets[1]];
			next_offset = offsets[1];

			if (delim_capture) {
				int i, match_len;
				for (i = 1; i < count; i++) {
					match_len = offsets[(i<<1)+1] - offsets[i<<1];
					/* If we have matched a delimiter */
					if (!no_empty || match_len > 0) {
						if (offset_capture) {
							add_offset_pair(return_value, &subject[offsets[i<<1]], match_len, offsets[i<<1], NULL);
						} else {
							add_next_index_stringl(return_value,
												   &subject[offsets[i<<1]],
												   match_len, 1);
						}
					}
				}
			}
		} else if (count == PCRE_ERROR_NOMATCH) {
			/* If we previously set PCRE_NOTEMPTY after a null match,
			   this is not necessarily the end. We need to advance
			   the start offset, and continue. Fudge the offset values
			   to achieve this, unless we're already at the end of the string. */
			if (g_notempty != 0 && start_offset < subject_len) {
				if (pce->compile_options & PCRE_UTF8) {
					if (re_bump == NULL) {
						int dummy;

						if ((re_bump = pcre_get_compiled_regex("/./us", &extra_bump, &dummy TSRMLS_CC)) == NULL) {
							RETURN_FALSE;
						}
					}
					count = pcre_exec(re_bump, extra_bump, subject,
							  subject_len, start_offset,
							  exoptions, offsets, size_offsets);
					if (count < 1) {
						php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown error");
						RETURN_FALSE;
					}
				} else {
					offsets[0] = start_offset;
					offsets[1] = start_offset + 1;
				}
			} else
				break;
		} else {
			pcre_handle_exec_error(count TSRMLS_CC);
			break;
		}

		/* If we have matched an empty string, mimic what Perl's /g options does.
		   This turns out to be rather cunning. First we set PCRE_NOTEMPTY and try
		   the match again at the same point. If this fails (picked up above) we
		   advance to the next character. */
		g_notempty = (offsets[1] == offsets[0])? PCRE_NOTEMPTY | PCRE_ANCHORED : 0;
		
		/* Advance to the position right after the last full match */
		start_offset = offsets[1];
	}


	start_offset = last_match - subject; /* the offset might have been incremented, but without further successful matches */

	if (!no_empty || start_offset < subject_len)
	{
		if (offset_capture) {
			/* Add the last (match, offset) pair to the return value */
			add_offset_pair(return_value, &subject[start_offset], subject_len - start_offset, start_offset, NULL);
		} else {
			/* Add the last piece to the return value */
			add_next_index_stringl(return_value, last_match, subject + subject_len - last_match, 1);
		}
	}

	
	/* Clean up */
	efree(offsets);
}