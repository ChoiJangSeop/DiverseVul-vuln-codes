apprentice_map(struct magic_set *ms, const char *fn)
{
	uint32_t *ptr;
	uint32_t version, entries, nentries;
	int needsbyteswap;
	char *dbname = NULL;
	struct magic_map *map;
	size_t i;
	php_stream *stream = NULL;
	php_stream_statbuf st;


	TSRMLS_FETCH();

	if ((map = CAST(struct magic_map *, ecalloc(1, sizeof(*map)))) == NULL) {
		file_oomem(ms, sizeof(*map));
		efree(map);
		goto error;
	}

	if (fn == NULL) {
		map->p = (void *)&php_magic_database;
		goto internal_loaded;
	}

#ifdef PHP_WIN32
	/* Don't bother on windows with php_stream_open_wrapper,
	return to give apprentice_load() a chance. */
	if (php_stream_stat_path_ex((char *)fn, 0, &st, NULL) == SUCCESS) {
               if (st.sb.st_mode & S_IFDIR) {
                       goto error;
               }
       }
#endif

	dbname = mkdbname(ms, fn, 0);
	if (dbname == NULL)
		goto error;

#if PHP_API_VERSION < 20100412
		stream = php_stream_open_wrapper((char *)fn, "rb", REPORT_ERRORS|ENFORCE_SAFE_MODE, NULL);
#else
		stream = php_stream_open_wrapper((char *)fn, "rb", REPORT_ERRORS, NULL);
#endif

	if (!stream) {
		goto error;
	}

	if (php_stream_stat(stream, &st) < 0) {
		file_error(ms, errno, "cannot stat `%s'", dbname);
		goto error;
	}

	if (st.sb.st_size < 8) {
		file_error(ms, 0, "file `%s' is too small", dbname);
		goto error;
	}

	map->len = (size_t)st.sb.st_size;
	if ((map->p = CAST(void *, emalloc(map->len))) == NULL) {
		file_oomem(ms, map->len);
		goto error;
	}
	if (php_stream_read(stream, map->p, (size_t)st.sb.st_size) != (size_t)st.sb.st_size) {
		file_badread(ms);
		goto error;
	}
	map->len = 0;
#define RET	1

	php_stream_close(stream);
	stream = NULL;

internal_loaded:
	ptr = (uint32_t *)(void *)map->p;
	if (*ptr != MAGICNO) {
		if (swap4(*ptr) != MAGICNO) {
			file_error(ms, 0, "bad magic in `%s'", dbname);
			goto error;
		}
		needsbyteswap = 1;
	} else
		needsbyteswap = 0;
	if (needsbyteswap)
		version = swap4(ptr[1]);
	else
		version = ptr[1];
	if (version != VERSIONNO) {
		file_error(ms, 0, "File %d.%d supports only version %d magic "
		    "files. `%s' is version %d", FILE_VERSION_MAJOR, patchlevel,
		    VERSIONNO, dbname, version);
		goto error;
	}

	/* php_magic_database is a const, performing writes will segfault. This is for big-endian
	machines only, PPC and Sparc specifically. Consider static variable or MINIT in
	future. */
	if (needsbyteswap && fn == NULL) {
		map->p = emalloc(sizeof(php_magic_database));
		map->p = memcpy(map->p, php_magic_database, sizeof(php_magic_database));
	}

	if (NULL != fn) {
		nentries = (uint32_t)(st.sb.st_size / sizeof(struct magic));
		entries = (uint32_t)(st.sb.st_size / sizeof(struct magic));
		if ((off_t)(entries * sizeof(struct magic)) != st.sb.st_size) {
			file_error(ms, 0, "Size of `%s' %llu is not a multiple of %zu",
				dbname, (unsigned long long)st.sb.st_size,
				sizeof(struct magic));
			goto error;
		}
	}
	map->magic[0] = CAST(struct magic *, map->p) + 1;
	nentries = 0;
	for (i = 0; i < MAGIC_SETS; i++) {
		if (needsbyteswap)
			map->nmagic[i] = swap4(ptr[i + 2]);
		else
			map->nmagic[i] = ptr[i + 2];
		if (i != MAGIC_SETS - 1)
			map->magic[i + 1] = map->magic[i] + map->nmagic[i];
		nentries += map->nmagic[i];
	}
	if (NULL != fn && entries != nentries + 1) {
		file_error(ms, 0, "Inconsistent entries in `%s' %u != %u",
		    dbname, entries, nentries + 1);
		goto error;
	}

	if (needsbyteswap)
		for (i = 0; i < MAGIC_SETS; i++)
			byteswap(map->magic[i], map->nmagic[i]);

	if (dbname) {
		efree(dbname);
	}
	return map;

error:
	if (stream) {
		php_stream_close(stream);
	}
	apprentice_unmap(map);
	if (dbname) {
		efree(dbname);
	}
	return NULL;
}