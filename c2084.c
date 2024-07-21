PHP_FUNCTION(get_meta_tags)
{
	char *filename;
	int filename_len;
	zend_bool use_include_path = 0;
	int in_tag = 0, done = 0;
	int looking_for_val = 0, have_name = 0, have_content = 0;
	int saw_name = 0, saw_content = 0;
	char *name = NULL, *value = NULL, *temp = NULL;
	php_meta_tags_token tok, tok_last;
	php_meta_tags_data md;

	/* Initiailize our structure */
	memset(&md, 0, sizeof(md));

	/* Parse arguments */
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|b", &filename, &filename_len, &use_include_path) == FAILURE) {
		return;
	}

	md.stream = php_stream_open_wrapper(filename, "rb",
			(use_include_path ? USE_PATH : 0) | ENFORCE_SAFE_MODE | REPORT_ERRORS,
			NULL);
	if (!md.stream)	{
		RETURN_FALSE;
	}

	array_init(return_value);

	tok_last = TOK_EOF;

	while (!done && (tok = php_next_meta_token(&md TSRMLS_CC)) != TOK_EOF) {
		if (tok == TOK_ID) {
			if (tok_last == TOK_OPENTAG) {
				md.in_meta = !strcasecmp("meta", md.token_data);
			} else if (tok_last == TOK_SLASH && in_tag) {
				if (strcasecmp("head", md.token_data) == 0) {
					/* We are done here! */
					done = 1;
				}
			} else if (tok_last == TOK_EQUAL && looking_for_val) {
				if (saw_name) {
					STR_FREE(name);
					/* Get the NAME attr (Single word attr, non-quoted) */
					temp = name = estrndup(md.token_data, md.token_len);

					while (temp && *temp) {
						if (strchr(PHP_META_UNSAFE, *temp)) {
							*temp = '_';
						}
						temp++;
					}

					have_name = 1;
				} else if (saw_content) {
					STR_FREE(value);
					/* Get the CONTENT attr (Single word attr, non-quoted) */
					if (PG(magic_quotes_runtime)) {
						value = php_addslashes(md.token_data, 0, &md.token_len, 0 TSRMLS_CC);
					} else {
						value = estrndup(md.token_data, md.token_len);
					}

					have_content = 1;
				}

				looking_for_val = 0;
			} else {
				if (md.in_meta) {
					if (strcasecmp("name", md.token_data) == 0) {
						saw_name = 1;
						saw_content = 0;
						looking_for_val = 1;
					} else if (strcasecmp("content", md.token_data) == 0) {
						saw_name = 0;
						saw_content = 1;
						looking_for_val = 1;
					}
				}
			}
		} else if (tok == TOK_STRING && tok_last == TOK_EQUAL && looking_for_val) {
			if (saw_name) {
				STR_FREE(name);
				/* Get the NAME attr (Quoted single/double) */
				temp = name = estrndup(md.token_data, md.token_len);

				while (temp && *temp) {
					if (strchr(PHP_META_UNSAFE, *temp)) {
						*temp = '_';
					}
					temp++;
				}

				have_name = 1;
			} else if (saw_content) {
				STR_FREE(value);
				/* Get the CONTENT attr (Single word attr, non-quoted) */
				if (PG(magic_quotes_runtime)) {
					value = php_addslashes(md.token_data, 0, &md.token_len, 0 TSRMLS_CC);
				} else {
					value = estrndup(md.token_data, md.token_len);
				}

				have_content = 1;
			}

			looking_for_val = 0;
		} else if (tok == TOK_OPENTAG) {
			if (looking_for_val) {
				looking_for_val = 0;
				have_name = saw_name = 0;
				have_content = saw_content = 0;
			}
			in_tag = 1;
		} else if (tok == TOK_CLOSETAG) {
			if (have_name) {
				/* For BC */
				php_strtolower(name, strlen(name));
				if (have_content) {
					add_assoc_string(return_value, name, value, 1);
				} else {
					add_assoc_string(return_value, name, "", 1);
				}

				efree(name);
				STR_FREE(value);
			} else if (have_content) {
				efree(value);
			}

			name = value = NULL;

			/* Reset all of our flags */
			in_tag = looking_for_val = 0;
			have_name = saw_name = 0;
			have_content = saw_content = 0;
			md.in_meta = 0;
		}

		tok_last = tok;

		if (md.token_data)
			efree(md.token_data);

		md.token_data = NULL;
	}

	STR_FREE(value);
	STR_FREE(name);
	php_stream_close(md.stream);
}