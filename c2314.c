apprentice_load(struct magic_set *ms, const char *fn, int action)
{
	int errs = 0;
	uint32_t i, j;
	size_t files = 0, maxfiles = 0;
	char **filearr = NULL;
	struct stat st;
	struct magic_map *map;
	struct magic_entry_set mset[MAGIC_SETS];
	php_stream *dir;
	php_stream_dirent d;
 
	TSRMLS_FETCH();

	memset(mset, 0, sizeof(mset));
	ms->flags |= MAGIC_CHECK;	/* Enable checks for parsed files */


	if ((map = CAST(struct magic_map *, ecalloc(1, sizeof(*map)))) == NULL)
	{
		file_oomem(ms, sizeof(*map));
		return NULL;
	}

	/* print silly verbose header for USG compat. */
	if (action == FILE_CHECK)
		(void)fprintf(stderr, "%s\n", usg_hdr);

	/* load directory or file */
	/* FIXME: Read file names and sort them to prevent
	   non-determinism. See Debian bug #488562. */
	if (php_sys_stat(fn, &st) == 0 && S_ISDIR(st.st_mode)) {
		int mflen;
		char mfn[MAXPATHLEN];

		dir = php_stream_opendir((char *)fn, REPORT_ERRORS, NULL);
		if (!dir) {
			errs++;
			goto out;
		}
		while (php_stream_readdir(dir, &d)) {
			if ((mflen = snprintf(mfn, sizeof(mfn), "%s/%s", fn, d.d_name)) < 0) {
				file_oomem(ms,
				strlen(fn) + strlen(d.d_name) + 2);
				errs++;
				php_stream_closedir(dir);
				goto out;
			}
			if (stat(mfn, &st) == -1 || !S_ISREG(st.st_mode)) {
				continue;
			}
			if (files >= maxfiles) {
				size_t mlen;
				maxfiles = (maxfiles + 1) * 2;
				mlen = maxfiles * sizeof(*filearr);
				if ((filearr = CAST(char **,
				    erealloc(filearr, mlen))) == NULL) {
					file_oomem(ms, mlen);
					efree(mfn);
					php_stream_closedir(dir);
					errs++;
					goto out;
				}
			}
			filearr[files++] = estrndup(mfn, (mflen > sizeof(mfn) - 1)? sizeof(mfn) - 1: mflen);
		}
		php_stream_closedir(dir);
		qsort(filearr, files, sizeof(*filearr), cmpstrp);
		for (i = 0; i < files; i++) {
			load_1(ms, action, filearr[i], &errs, mset);
			efree(filearr[i]);
		}
		efree(filearr);
	} else
		load_1(ms, action, fn, &errs, mset);
	if (errs)
		goto out;

	for (j = 0; j < MAGIC_SETS; j++) {
		/* Set types of tests */
		for (i = 0; i < mset[j].count; ) {
			if (mset[j].me[i].mp->cont_level != 0) {
				i++;
				continue;
			}
			i = set_text_binary(ms, mset[j].me, mset[j].count, i);
		}
		qsort(mset[j].me, mset[j].count, sizeof(*mset[j].me),
		    apprentice_sort);

		/*
		 * Make sure that any level 0 "default" line is last
		 * (if one exists).
		 */
		set_last_default(ms, mset[j].me, mset[j].count);

		/* coalesce per file arrays into a single one */
		if (coalesce_entries(ms, mset[j].me, mset[j].count,
		    &map->magic[j], &map->nmagic[j]) == -1) {
			errs++;
			goto out;
		}
	}

out:
	for (j = 0; j < MAGIC_SETS; j++)
		magic_entry_free(mset[j].me, mset[j].count);

	if (errs) {
		for (j = 0; j < MAGIC_SETS; j++) {
			if (map->magic[j])
				efree(map->magic[j]);
		}
		efree(map);
		return NULL;
	}
	return map;
}