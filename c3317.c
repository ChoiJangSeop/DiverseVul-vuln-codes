static int ldb_wildcard_compare(struct ldb_context *ldb,
				const struct ldb_parse_tree *tree,
				const struct ldb_val value, bool *matched)
{
	const struct ldb_schema_attribute *a;
	struct ldb_val val;
	struct ldb_val cnk;
	struct ldb_val *chunk;
	char *p, *g;
	uint8_t *save_p = NULL;
	unsigned int c = 0;

	a = ldb_schema_attribute_by_name(ldb, tree->u.substring.attr);
	if (!a) {
		return LDB_ERR_INVALID_ATTRIBUTE_SYNTAX;
	}

	if (tree->u.substring.chunks == NULL) {
		*matched = false;
		return LDB_SUCCESS;
	}

	if (a->syntax->canonicalise_fn(ldb, ldb, &value, &val) != 0) {
		return LDB_ERR_INVALID_ATTRIBUTE_SYNTAX;
	}

	save_p = val.data;
	cnk.data = NULL;

	if ( ! tree->u.substring.start_with_wildcard ) {

		chunk = tree->u.substring.chunks[c];
		if (a->syntax->canonicalise_fn(ldb, ldb, chunk, &cnk) != 0) goto mismatch;

		/* This deals with wildcard prefix searches on binary attributes (eg objectGUID) */
		if (cnk.length > val.length) {
			goto mismatch;
		}
		/*
		 * Empty strings are returned as length 0. Ensure
		 * we can cope with this.
		 */
		if (cnk.length == 0) {
			goto mismatch;
		}

		if (memcmp((char *)val.data, (char *)cnk.data, cnk.length) != 0) goto mismatch;
		val.length -= cnk.length;
		val.data += cnk.length;
		c++;
		talloc_free(cnk.data);
		cnk.data = NULL;
	}

	while (tree->u.substring.chunks[c]) {

		chunk = tree->u.substring.chunks[c];
		if(a->syntax->canonicalise_fn(ldb, ldb, chunk, &cnk) != 0) goto mismatch;

		/*
		 * Empty strings are returned as length 0. Ensure
		 * we can cope with this.
		 */
		if (cnk.length == 0) {
			goto mismatch;
		}
		p = strstr((char *)val.data, (char *)cnk.data);
		if (p == NULL) goto mismatch;
		if ( (! tree->u.substring.chunks[c + 1]) && (! tree->u.substring.end_with_wildcard) ) {
			do { /* greedy */
				g = strstr((char *)p + cnk.length, (char *)cnk.data);
				if (g) p = g;
			} while(g);
		}
		val.length = val.length - (p - (char *)(val.data)) - cnk.length;
		val.data = (uint8_t *)(p + cnk.length);
		c++;
		talloc_free(cnk.data);
		cnk.data = NULL;
	}

	/* last chunk may not have reached end of string */
	if ( (! tree->u.substring.end_with_wildcard) && (*(val.data) != 0) ) goto mismatch;
	talloc_free(save_p);
	*matched = true;
	return LDB_SUCCESS;

mismatch:
	*matched = false;
	talloc_free(save_p);
	talloc_free(cnk.data);
	return LDB_SUCCESS;
}