PHPAPI char *php_pcre_replace_impl(pcre_cache_entry *pce, char *subject, int subject_len, zval *replace_val, 
	int is_callable_replace, int *result_len, int limit, int *replace_count TSRMLS_DC)
{
	pcre_extra		*extra = pce->extra;/* Holds results of studying */
	pcre_extra		 extra_data;		/* Used locally for exec options */
	int				 exoptions = 0;		/* Execution options */
	int				 count = 0;			/* Count of matched subpatterns */
	int				*offsets;			/* Array of subpattern offsets */
	char 			**subpat_names;		/* Array for named subpatterns */
	int				 num_subpats;		/* Number of captured subpatterns */
	int				 size_offsets;		/* Size of the offsets array */
	int				 new_len;			/* Length of needed storage */
	int				 alloc_len;			/* Actual allocated length */
	int				 eval_result_len=0;	/* Length of the eval'ed or
										   function-returned string */
	int				 match_len;			/* Length of the current match */
	int				 backref;			/* Backreference number */
	int				 eval;				/* If the replacement string should be eval'ed */
	int				 start_offset;		/* Where the new search starts */
	int				 g_notempty=0;		/* If the match should not be empty */
	int				 replace_len=0;		/* Length of replacement string */
	char			*result,			/* Result of replacement */
					*replace=NULL,		/* Replacement string */
					*new_buf,			/* Temporary buffer for re-allocation */
					*walkbuf,			/* Location of current replacement in the result */
					*walk,				/* Used to walk the replacement string */
					*match,				/* The current match */
					*piece,				/* The current piece of subject */
					*replace_end=NULL,	/* End of replacement string */
					*eval_result,		/* Result of eval or custom function */
					 walk_last;			/* Last walked character */
	int				 rc;

	if (extra == NULL) {
		extra_data.flags = PCRE_EXTRA_MATCH_LIMIT | PCRE_EXTRA_MATCH_LIMIT_RECURSION;
		extra = &extra_data;
	}
	extra->match_limit = PCRE_G(backtrack_limit);
	extra->match_limit_recursion = PCRE_G(recursion_limit);

	eval = pce->preg_options & PREG_REPLACE_EVAL;
	if (is_callable_replace) {
		if (eval) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Modifier /e cannot be used with replacement callback");
			return NULL;
		}
	} else {
		replace = Z_STRVAL_P(replace_val);
		replace_len = Z_STRLEN_P(replace_val);
		replace_end = replace + replace_len;
	}

	/* Calculate the size of the offsets array, and allocate memory for it. */
	rc = pcre_fullinfo(pce->re, extra, PCRE_INFO_CAPTURECOUNT, &num_subpats);
	if (rc < 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Internal pcre_fullinfo() error %d", rc);
		return NULL;
	}
	num_subpats++;
	size_offsets = num_subpats * 3;

	/*
	 * Build a mapping from subpattern numbers to their names. We will always
	 * allocate the table, even though there may be no named subpatterns. This
	 * avoids somewhat more complicated logic in the inner loops.
	 */
	subpat_names = make_subpats_table(num_subpats, pce TSRMLS_CC);
	if (!subpat_names) {
		return NULL;
	}

	offsets = (int *)safe_emalloc(size_offsets, sizeof(int), 0);
	
	alloc_len = 2 * subject_len + 1;
	result = safe_emalloc(alloc_len, sizeof(char), 0);

	/* Initialize */
	match = NULL;
	*result_len = 0;
	start_offset = 0;
	PCRE_G(error_code) = PHP_PCRE_NO_ERROR;
	
	while (1) {
		/* Execute the regular expression. */
		count = pcre_exec(pce->re, extra, subject, subject_len, start_offset,
						  exoptions|g_notempty, offsets, size_offsets);

		/* the string was already proved to be valid UTF-8 */
		exoptions |= PCRE_NO_UTF8_CHECK;

		/* Check for too many substrings condition. */
		if (count == 0) {
			php_error_docref(NULL TSRMLS_CC,E_NOTICE, "Matched, but too many substrings");
			count = size_offsets/3;
		}

		piece = subject + start_offset;

		if (count > 0 && (limit == -1 || limit > 0)) {
			if (replace_count) {
				++*replace_count;
			}
			/* Set the match location in subject */
			match = subject + offsets[0];

			new_len = *result_len + offsets[0] - start_offset; /* part before the match */
			
			/* If evaluating, do it and add the return string's length */
			if (eval) {
				eval_result_len = preg_do_eval(replace, replace_len, subject,
											   offsets, count, &eval_result TSRMLS_CC);
				new_len += eval_result_len;
			} else if (is_callable_replace) {
				/* Use custom function to get replacement string and its length. */
				eval_result_len = preg_do_repl_func(replace_val, subject, offsets, subpat_names, count, &eval_result TSRMLS_CC);
				new_len += eval_result_len;
			} else { /* do regular substitution */
				walk = replace;
				walk_last = 0;
				while (walk < replace_end) {
					if ('\\' == *walk || '$' == *walk) {
						if (walk_last == '\\') {
							walk++;
							walk_last = 0;
							continue;
						}
						if (preg_get_backref(&walk, &backref)) {
							if (backref < count)
								new_len += offsets[(backref<<1)+1] - offsets[backref<<1];
							continue;
						}
					}
					new_len++;
					walk++;
					walk_last = walk[-1];
				}
			}

			if (new_len + 1 > alloc_len) {
				alloc_len = 1 + alloc_len + 2 * new_len;
				new_buf = emalloc(alloc_len);
				memcpy(new_buf, result, *result_len);
				efree(result);
				result = new_buf;
			}
			/* copy the part of the string before the match */
			memcpy(&result[*result_len], piece, match-piece);
			*result_len += match-piece;

			/* copy replacement and backrefs */
			walkbuf = result + *result_len;
			
			/* If evaluating or using custom function, copy result to the buffer
			 * and clean up. */
			if (eval || is_callable_replace) {
				memcpy(walkbuf, eval_result, eval_result_len);
				*result_len += eval_result_len;
				STR_FREE(eval_result);
			} else { /* do regular backreference copying */
				walk = replace;
				walk_last = 0;
				while (walk < replace_end) {
					if ('\\' == *walk || '$' == *walk) {
						if (walk_last == '\\') {
							*(walkbuf-1) = *walk++;
							walk_last = 0;
							continue;
						}
						if (preg_get_backref(&walk, &backref)) {
							if (backref < count) {
								match_len = offsets[(backref<<1)+1] - offsets[backref<<1];
								memcpy(walkbuf, subject + offsets[backref<<1], match_len);
								walkbuf += match_len;
							}
							continue;
						}
					}
					*walkbuf++ = *walk++;
					walk_last = walk[-1];
				}
				*walkbuf = '\0';
				/* increment the result length by how much we've added to the string */
				*result_len += walkbuf - (result + *result_len);
			}

			if (limit != -1)
				limit--;

		} else if (count == PCRE_ERROR_NOMATCH || limit == 0) {
			/* If we previously set PCRE_NOTEMPTY after a null match,
			   this is not necessarily the end. We need to advance
			   the start offset, and continue. Fudge the offset values
			   to achieve this, unless we're already at the end of the string. */
			if (g_notempty != 0 && start_offset < subject_len) {
				offsets[0] = start_offset;
				offsets[1] = start_offset + 1;
				memcpy(&result[*result_len], piece, 1);
				(*result_len)++;
			} else {
				new_len = *result_len + subject_len - start_offset;
				if (new_len + 1 > alloc_len) {
					alloc_len = new_len + 1; /* now we know exactly how long it is */
					new_buf = safe_emalloc(alloc_len, sizeof(char), 0);
					memcpy(new_buf, result, *result_len);
					efree(result);
					result = new_buf;
				}
				/* stick that last bit of string on our output */
				memcpy(&result[*result_len], piece, subject_len - start_offset);
				*result_len += subject_len - start_offset;
				result[*result_len] = '\0';
				break;
			}
		} else {
			pcre_handle_exec_error(count TSRMLS_CC);
			efree(result);
			result = NULL;
			break;
		}
			
		/* If we have matched an empty string, mimic what Perl's /g options does.
		   This turns out to be rather cunning. First we set PCRE_NOTEMPTY and try
		   the match again at the same point. If this fails (picked up above) we
		   advance to the next character. */
		g_notempty = (offsets[1] == offsets[0])? PCRE_NOTEMPTY | PCRE_ANCHORED : 0;
		
		/* Advance to the next piece. */
		start_offset = offsets[1];
	}

	efree(offsets);
	efree(subpat_names);

	return result;
}