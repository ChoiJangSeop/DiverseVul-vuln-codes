archive_read_format_lha_read_header(struct archive_read *a,
    struct archive_entry *entry)
{
	struct archive_string linkname;
	struct archive_string pathname;
	struct lha *lha;
	const unsigned char *p;
	const char *signature;
	int err;
	
	lha_crc16_init();

	a->archive.archive_format = ARCHIVE_FORMAT_LHA;
	if (a->archive.archive_format_name == NULL)
		a->archive.archive_format_name = "lha";

	lha = (struct lha *)(a->format->data);
	lha->decompress_init = 0;
	lha->end_of_entry = 0;
	lha->end_of_entry_cleanup = 0;
	lha->entry_unconsumed = 0;

	if ((p = __archive_read_ahead(a, H_SIZE, NULL)) == NULL) {
		/*
		 * LHa archiver added 0 to the tail of its archive file as
		 * the mark of the end of the archive.
		 */
		signature = __archive_read_ahead(a, sizeof(signature[0]), NULL);
		if (signature == NULL || signature[0] == 0)
			return (ARCHIVE_EOF);
		return (truncated_error(a));
	}

	signature = (const char *)p;
	if (lha->found_first_header == 0 &&
	    signature[0] == 'M' && signature[1] == 'Z') {
                /* This is an executable?  Must be self-extracting... 	*/
		err = lha_skip_sfx(a);
		if (err < ARCHIVE_WARN)
			return (err);

		if ((p = __archive_read_ahead(a, sizeof(*p), NULL)) == NULL)
			return (truncated_error(a));
		signature = (const char *)p;
	}
	/* signature[0] == 0 means the end of an LHa archive file. */
	if (signature[0] == 0)
		return (ARCHIVE_EOF);

	/*
	 * Check the header format and method type.
	 */
	if (lha_check_header_format(p) != 0) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
		    "Bad LHa file");
		return (ARCHIVE_FATAL);
	}

	/* We've found the first header. */
	lha->found_first_header = 1;
	/* Set a default value and common data */
	lha->header_size = 0;
	lha->level = p[H_LEVEL_OFFSET];
	lha->method[0] = p[H_METHOD_OFFSET+1];
	lha->method[1] = p[H_METHOD_OFFSET+2];
	lha->method[2] = p[H_METHOD_OFFSET+3];
	if (memcmp(lha->method, "lhd", 3) == 0)
		lha->directory = 1;
	else
		lha->directory = 0;
	if (memcmp(lha->method, "lh0", 3) == 0 ||
	    memcmp(lha->method, "lz4", 3) == 0)
		lha->entry_is_compressed = 0;
	else
		lha->entry_is_compressed = 1;

	lha->compsize = 0;
	lha->origsize = 0;
	lha->setflag = 0;
	lha->birthtime = 0;
	lha->birthtime_tv_nsec = 0;
	lha->mtime = 0;
	lha->mtime_tv_nsec = 0;
	lha->atime = 0;
	lha->atime_tv_nsec = 0;
	lha->mode = (lha->directory)? 0777 : 0666;
	lha->uid = 0;
	lha->gid = 0;
	archive_string_empty(&lha->dirname);
	archive_string_empty(&lha->filename);
	lha->dos_attr = 0;
	if (lha->opt_sconv != NULL)
		lha->sconv = lha->opt_sconv;
	else
		lha->sconv = NULL;

	switch (p[H_LEVEL_OFFSET]) {
	case 0:
		err = lha_read_file_header_0(a, lha);
		break;
	case 1:
		err = lha_read_file_header_1(a, lha);
		break;
	case 2:
		err = lha_read_file_header_2(a, lha);
		break;
	case 3:
		err = lha_read_file_header_3(a, lha);
		break;
	default:
		archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
		    "Unsupported LHa header level %d", p[H_LEVEL_OFFSET]);
		err = ARCHIVE_FATAL;
		break;
	}
	if (err < ARCHIVE_WARN)
		return (err);


	if (!lha->directory && archive_strlen(&lha->filename) == 0)
		/* The filename has not been set */
		return (truncated_error(a));

	/*
	 * Make a pathname from a dirname and a filename.
	 */
	archive_string_concat(&lha->dirname, &lha->filename);
	archive_string_init(&pathname);
	archive_string_init(&linkname);
	archive_string_copy(&pathname, &lha->dirname);

	if ((lha->mode & AE_IFMT) == AE_IFLNK) {
		/*
	 	 * Extract the symlink-name if it's included in the pathname.
	 	 */
		if (!lha_parse_linkname(&linkname, &pathname)) {
			/* We couldn't get the symlink-name. */
			archive_set_error(&a->archive,
		    	    ARCHIVE_ERRNO_FILE_FORMAT,
			    "Unknown symlink-name");
			archive_string_free(&pathname);
			archive_string_free(&linkname);
			return (ARCHIVE_FAILED);
		}
	} else {
		/*
		 * Make sure a file-type is set.
		 * The mode has been overridden if it is in the extended data.
		 */
		lha->mode = (lha->mode & ~AE_IFMT) |
		    ((lha->directory)? AE_IFDIR: AE_IFREG);
	}
	if ((lha->setflag & UNIX_MODE_IS_SET) == 0 &&
	    (lha->dos_attr & 1) != 0)
		lha->mode &= ~(0222);/* read only. */

	/*
	 * Set basic file parameters.
	 */
	if (archive_entry_copy_pathname_l(entry, pathname.s,
	    pathname.length, lha->sconv) != 0) {
		if (errno == ENOMEM) {
			archive_set_error(&a->archive, ENOMEM,
			    "Can't allocate memory for Pathname");
			return (ARCHIVE_FATAL);
		}
		archive_set_error(&a->archive,
		    ARCHIVE_ERRNO_FILE_FORMAT,
		    "Pathname cannot be converted "
		    "from %s to current locale.",
		    archive_string_conversion_charset_name(lha->sconv));
		err = ARCHIVE_WARN;
	}
	archive_string_free(&pathname);
	if (archive_strlen(&linkname) > 0) {
		if (archive_entry_copy_symlink_l(entry, linkname.s,
		    linkname.length, lha->sconv) != 0) {
			if (errno == ENOMEM) {
				archive_set_error(&a->archive, ENOMEM,
				    "Can't allocate memory for Linkname");
				return (ARCHIVE_FATAL);
			}
			archive_set_error(&a->archive,
			    ARCHIVE_ERRNO_FILE_FORMAT,
			    "Linkname cannot be converted "
			    "from %s to current locale.",
			    archive_string_conversion_charset_name(lha->sconv));
			err = ARCHIVE_WARN;
		}
	} else
		archive_entry_set_symlink(entry, NULL);
	archive_string_free(&linkname);
	/*
	 * When a header level is 0, there is a possibility that
	 * a pathname and a symlink has '\' character, a directory
	 * separator in DOS/Windows. So we should convert it to '/'.
	 */
	if (p[H_LEVEL_OFFSET] == 0)
		lha_replace_path_separator(lha, entry);

	archive_entry_set_mode(entry, lha->mode);
	archive_entry_set_uid(entry, lha->uid);
	archive_entry_set_gid(entry, lha->gid);
	if (archive_strlen(&lha->uname) > 0)
		archive_entry_set_uname(entry, lha->uname.s);
	if (archive_strlen(&lha->gname) > 0)
		archive_entry_set_gname(entry, lha->gname.s);
	if (lha->setflag & BIRTHTIME_IS_SET) {
		archive_entry_set_birthtime(entry, lha->birthtime,
		    lha->birthtime_tv_nsec);
		archive_entry_set_ctime(entry, lha->birthtime,
		    lha->birthtime_tv_nsec);
	} else {
		archive_entry_unset_birthtime(entry);
		archive_entry_unset_ctime(entry);
	}
	archive_entry_set_mtime(entry, lha->mtime, lha->mtime_tv_nsec);
	if (lha->setflag & ATIME_IS_SET)
		archive_entry_set_atime(entry, lha->atime,
		    lha->atime_tv_nsec);
	else
		archive_entry_unset_atime(entry);
	if (lha->directory || archive_entry_symlink(entry) != NULL)
		archive_entry_unset_size(entry);
	else
		archive_entry_set_size(entry, lha->origsize);

	/*
	 * Prepare variables used to read a file content.
	 */
	lha->entry_bytes_remaining = lha->compsize;
	lha->entry_offset = 0;
	lha->entry_crc_calculated = 0;

	/*
	 * This file does not have a content.
	 */
	if (lha->directory || lha->compsize == 0)
		lha->end_of_entry = 1;

	sprintf(lha->format_name, "lha -%c%c%c-",
	    lha->method[0], lha->method[1], lha->method[2]);
	a->archive.archive_format_name = lha->format_name;

	return (err);
}