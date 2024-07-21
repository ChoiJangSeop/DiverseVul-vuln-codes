static int phar_tar_setupmetadata(void *pDest, void *argument TSRMLS_DC) /* {{{ */
{
	int lookfor_len;
	struct _phar_pass_tar_info *i = (struct _phar_pass_tar_info *)argument;
	char *lookfor, **error = i->error;
	phar_entry_info *entry = (phar_entry_info *)pDest, *metadata, newentry = {0};

	if (entry->filename_len >= sizeof(".phar/.metadata") && !memcmp(entry->filename, ".phar/.metadata", sizeof(".phar/.metadata")-1)) {
		if (entry->filename_len == sizeof(".phar/.metadata.bin")-1 && !memcmp(entry->filename, ".phar/.metadata.bin", sizeof(".phar/.metadata.bin")-1)) {
			return phar_tar_setmetadata(entry->phar->metadata, entry, error TSRMLS_CC);
		}
		/* search for the file this metadata entry references */
		if (entry->filename_len >= sizeof(".phar/.metadata/") + sizeof("/.metadata.bin") - 1 && !zend_hash_exists(&(entry->phar->manifest), entry->filename + sizeof(".phar/.metadata/") - 1, entry->filename_len - (sizeof("/.metadata.bin") - 1 + sizeof(".phar/.metadata/") - 1))) {
			/* this is orphaned metadata, erase it */
			return ZEND_HASH_APPLY_REMOVE;
		}
		/* we can keep this entry, the file that refers to it exists */
		return ZEND_HASH_APPLY_KEEP;
	}

	if (!entry->is_modified) {
		return ZEND_HASH_APPLY_KEEP;
	}

	/* now we are dealing with regular files, so look for metadata */
	lookfor_len = spprintf(&lookfor, 0, ".phar/.metadata/%s/.metadata.bin", entry->filename);

	if (!entry->metadata) {
		zend_hash_del(&(entry->phar->manifest), lookfor, lookfor_len);
		efree(lookfor);
		return ZEND_HASH_APPLY_KEEP;
	}

	if (SUCCESS == zend_hash_find(&(entry->phar->manifest), lookfor, lookfor_len, (void **)&metadata)) {
		int ret;
		ret = phar_tar_setmetadata(entry->metadata, metadata, error TSRMLS_CC);
		efree(lookfor);
		return ret;
	}

	newentry.filename = lookfor;
	newentry.filename_len = lookfor_len;
	newentry.phar = entry->phar;
	newentry.tar_type = TAR_FILE;
	newentry.is_tar = 1;

	if (SUCCESS != zend_hash_add(&(entry->phar->manifest), lookfor, lookfor_len, (void *)&newentry, sizeof(phar_entry_info), (void **)&metadata)) {
		efree(lookfor);
		spprintf(error, 0, "phar tar error: unable to add magic metadata file to manifest for file \"%s\"", entry->filename);
		return ZEND_HASH_APPLY_STOP;
	}

	return phar_tar_setmetadata(entry->metadata, metadata, error TSRMLS_CC);
}