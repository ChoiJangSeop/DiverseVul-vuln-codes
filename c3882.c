PHP_FUNCTION(get_html_translation_table)
{
	long all = HTML_SPECIALCHARS,
		 flags = ENT_COMPAT;
	int doctype;
	entity_table_opt entity_table;
	const enc_to_uni *to_uni_table = NULL;
	char *charset_hint = NULL;
	int charset_hint_len;
	enum entity_charset charset;

	/* in this function we have to jump through some loops because we're
	 * getting the translated table from data structures that are optimized for
	 * random access, not traversal */

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|lls",
			&all, &flags, &charset_hint, &charset_hint_len) == FAILURE) {
		return;
	}

	charset = determine_charset(charset_hint TSRMLS_CC);
	doctype = flags & ENT_HTML_DOC_TYPE_MASK;
	LIMIT_ALL(all, doctype, charset);

	array_init(return_value);
	
	entity_table = determine_entity_table(all, doctype);
	if (all && !CHARSET_UNICODE_COMPAT(charset)) {
		to_uni_table = enc_to_uni_index[charset];
	}

	if (all) { /* HTML_ENTITIES (actually, any non-zero value for 1st param) */
		const entity_stage1_row *ms_table = entity_table.ms_table;

		if (CHARSET_UNICODE_COMPAT(charset)) {
			unsigned i, j, k,
					 max_i, max_j, max_k;
			/* no mapping to unicode required */
			if (CHARSET_SINGLE_BYTE(charset)) { /* ISO-8859-1 */
				max_i = 1; max_j = 4; max_k = 64;
			} else {
				max_i = 0x1E; max_j = 64; max_k = 64;
			}

			for (i = 0; i < max_i; i++) {
				if (ms_table[i] == empty_stage2_table)
					continue;
				for (j = 0; j < max_j; j++) {
					if (ms_table[i][j] == empty_stage3_table)
						continue;
					for (k = 0; k < max_k; k++) {
						const entity_stage3_row *r = &ms_table[i][j][k];
						unsigned code;

						if (r->data.ent.entity == NULL)
							continue;

						code = ENT_CODE_POINT_FROM_STAGES(i, j, k);
						if (((code == '\'' && !(flags & ENT_HTML_QUOTE_SINGLE)) ||
								(code == '"' && !(flags & ENT_HTML_QUOTE_DOUBLE))))
							continue;
						write_s3row_data(r, code, charset, return_value);
					}
				}
			}
		} else {
			/* we have to iterate through the set of code points for this
			 * encoding and map them to unicode code points */
			unsigned i;
			for (i = 0; i <= 0xFF; i++) {
				const entity_stage3_row *r;
				unsigned uni_cp;

				/* can be done before mapping, they're invariant */
				if (((i == '\'' && !(flags & ENT_HTML_QUOTE_SINGLE)) ||
						(i == '"' && !(flags & ENT_HTML_QUOTE_DOUBLE))))
					continue;

				map_to_unicode(i, to_uni_table, &uni_cp);
				r = &ms_table[ENT_STAGE1_INDEX(uni_cp)][ENT_STAGE2_INDEX(uni_cp)][ENT_STAGE3_INDEX(uni_cp)];
				if (r->data.ent.entity == NULL)
					continue;

				write_s3row_data(r, i, charset, return_value);
			}
		}
	} else {
		/* we could use sizeof(stage3_table_be_apos_00000) as well */
		unsigned	  j,
					  numelems = sizeof(stage3_table_be_noapos_00000) /
							sizeof(*stage3_table_be_noapos_00000);

		for (j = 0; j < numelems; j++) {
			const entity_stage3_row *r = &entity_table.table[j];
			if (r->data.ent.entity == NULL)
				continue;

			if (((j == '\'' && !(flags & ENT_HTML_QUOTE_SINGLE)) ||
					(j == '"' && !(flags & ENT_HTML_QUOTE_DOUBLE))))
				continue;

			/* charset is indifferent, used cs_8859_1 for efficiency */
			write_s3row_data(r, j, cs_8859_1, return_value);
		}
	}
}