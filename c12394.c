s64 ntfs_get_attribute_value(const ntfs_volume *vol,
		const ATTR_RECORD *a, u8 *b)
{
	runlist *rl;
	s64 total, r;
	int i;

	/* Sanity checks. */
	if (!vol || !a || !b) {
		errno = EINVAL;
		return 0;
	}
	/* Complex attribute? */
	/*
	 * Ignore the flags in case they are not zero for an attribute list
	 * attribute.  Windows does not complain about invalid flags and chkdsk
	 * does not detect or fix them so we need to cope with it, too.
	 */
	if (a->type != AT_ATTRIBUTE_LIST && a->flags) {
		ntfs_log_error("Non-zero (%04x) attribute flags. Cannot handle "
			       "this yet.\n", le16_to_cpu(a->flags));
		errno = EOPNOTSUPP;
		return 0;
	}
	if (!a->non_resident) {
		/* Attribute is resident. */

		/* Sanity check. */
		if (le32_to_cpu(a->value_length) + le16_to_cpu(a->value_offset)
				> le32_to_cpu(a->length)) {
			return 0;
		}

		memcpy(b, (const char*)a + le16_to_cpu(a->value_offset),
				le32_to_cpu(a->value_length));
		errno = 0;
		return (s64)le32_to_cpu(a->value_length);
	}

	/* Attribute is not resident. */

	/* If no data, return 0. */
	if (!(a->data_size)) {
		errno = 0;
		return 0;
	}
	/*
	 * FIXME: What about attribute lists?!? (AIA)
	 */
	/* Decompress the mapping pairs array into a runlist. */
	rl = ntfs_mapping_pairs_decompress(vol, a, NULL);
	if (!rl) {
		errno = EINVAL;
		return 0;
	}
	/*
	 * FIXED: We were overflowing here in a nasty fashion when we
	 * reach the last cluster in the runlist as the buffer will
	 * only be big enough to hold data_size bytes while we are
	 * reading in allocated_size bytes which is usually larger
	 * than data_size, since the actual data is unlikely to have a
	 * size equal to a multiple of the cluster size!
	 * FIXED2:  We were also overflowing here in the same fashion
	 * when the data_size was more than one run smaller than the
	 * allocated size which happens with Windows XP sometimes.
	 */
	/* Now load all clusters in the runlist into b. */
	for (i = 0, total = 0; rl[i].length; i++) {
		if (total + (rl[i].length << vol->cluster_size_bits) >=
				sle64_to_cpu(a->data_size)) {
			unsigned char *intbuf = NULL;
			/*
			 * We have reached the last run so we were going to
			 * overflow when executing the ntfs_pread() which is
			 * BAAAAAAAD!
			 * Temporary fix:
			 *	Allocate a new buffer with size:
			 *	rl[i].length << vol->cluster_size_bits, do the
			 *	read into our buffer, then memcpy the correct
			 *	amount of data into the caller supplied buffer,
			 *	free our buffer, and continue.
			 * We have reached the end of data size so we were
			 * going to overflow in the same fashion.
			 * Temporary fix:  same as above.
			 */
			intbuf = ntfs_malloc(rl[i].length << vol->cluster_size_bits);
			if (!intbuf) {
				free(rl);
				return 0;
			}
			/*
			 * FIXME: If compressed file: Only read if lcn != -1.
			 * Otherwise, we are dealing with a sparse run and we
			 * just memset the user buffer to 0 for the length of
			 * the run, which should be 16 (= compression unit
			 * size).
			 * FIXME: Really only when file is compressed, or can
			 * we have sparse runs in uncompressed files as well?
			 * - Yes we can, in sparse files! But not necessarily
			 * size of 16, just run length.
			 */
			r = ntfs_pread(vol->dev, rl[i].lcn <<
					vol->cluster_size_bits, rl[i].length <<
					vol->cluster_size_bits, intbuf);
			if (r != rl[i].length << vol->cluster_size_bits) {
#define ESTR "Error reading attribute value"
				if (r == -1)
					ntfs_log_perror(ESTR);
				else if (r < rl[i].length <<
						vol->cluster_size_bits) {
					ntfs_log_debug(ESTR ": Ran out of input data.\n");
					errno = EIO;
				} else {
					ntfs_log_debug(ESTR ": unknown error\n");
					errno = EIO;
				}
#undef ESTR
				free(rl);
				free(intbuf);
				return 0;
			}
			memcpy(b + total, intbuf, sle64_to_cpu(a->data_size) -
					total);
			free(intbuf);
			total = sle64_to_cpu(a->data_size);
			break;
		}
		/*
		 * FIXME: If compressed file: Only read if lcn != -1.
		 * Otherwise, we are dealing with a sparse run and we just
		 * memset the user buffer to 0 for the length of the run, which
		 * should be 16 (= compression unit size).
		 * FIXME: Really only when file is compressed, or can
		 * we have sparse runs in uncompressed files as well?
		 * - Yes we can, in sparse files! But not necessarily size of
		 * 16, just run length.
		 */
		r = ntfs_pread(vol->dev, rl[i].lcn << vol->cluster_size_bits,
				rl[i].length << vol->cluster_size_bits,
				b + total);
		if (r != rl[i].length << vol->cluster_size_bits) {
#define ESTR "Error reading attribute value"
			if (r == -1)
				ntfs_log_perror(ESTR);
			else if (r < rl[i].length << vol->cluster_size_bits) {
				ntfs_log_debug(ESTR ": Ran out of input data.\n");
				errno = EIO;
			} else {
				ntfs_log_debug(ESTR ": unknown error\n");
				errno = EIO;
			}
#undef ESTR
			free(rl);
			return 0;
		}
		total += r;
	}
	free(rl);
	return total;
}