static int log_read_rst(struct ntfs_log *log, u32 l_size, bool first,
			struct restart_info *info)
{
	u32 skip, vbo;
	struct RESTART_HDR *r_page = kmalloc(DefaultLogPageSize, GFP_NOFS);

	if (!r_page)
		return -ENOMEM;

	memset(info, 0, sizeof(struct restart_info));

	/* Determine which restart area we are looking for. */
	if (first) {
		vbo = 0;
		skip = 512;
	} else {
		vbo = 512;
		skip = 0;
	}

	/* Loop continuously until we succeed. */
	for (; vbo < l_size; vbo = 2 * vbo + skip, skip = 0) {
		bool usa_error;
		u32 sys_page_size;
		bool brst, bchk;
		struct RESTART_AREA *ra;

		/* Read a page header at the current offset. */
		if (read_log_page(log, vbo, (struct RECORD_PAGE_HDR **)&r_page,
				  &usa_error)) {
			/* Ignore any errors. */
			continue;
		}

		/* Exit if the signature is a log record page. */
		if (r_page->rhdr.sign == NTFS_RCRD_SIGNATURE) {
			info->initialized = true;
			break;
		}

		brst = r_page->rhdr.sign == NTFS_RSTR_SIGNATURE;
		bchk = r_page->rhdr.sign == NTFS_CHKD_SIGNATURE;

		if (!bchk && !brst) {
			if (r_page->rhdr.sign != NTFS_FFFF_SIGNATURE) {
				/*
				 * Remember if the signature does not
				 * indicate uninitialized file.
				 */
				info->initialized = true;
			}
			continue;
		}

		ra = NULL;
		info->valid_page = false;
		info->initialized = true;
		info->vbo = vbo;

		/* Let's check the restart area if this is a valid page. */
		if (!is_rst_page_hdr_valid(vbo, r_page))
			goto check_result;
		ra = Add2Ptr(r_page, le16_to_cpu(r_page->ra_off));

		if (!is_rst_area_valid(r_page))
			goto check_result;

		/*
		 * We have a valid restart page header and restart area.
		 * If chkdsk was run or we have no clients then we have
		 * no more checking to do.
		 */
		if (bchk || ra->client_idx[1] == LFS_NO_CLIENT_LE) {
			info->valid_page = true;
			goto check_result;
		}

		/* Read the entire restart area. */
		sys_page_size = le32_to_cpu(r_page->sys_page_size);
		if (DefaultLogPageSize != sys_page_size) {
			kfree(r_page);
			r_page = kzalloc(sys_page_size, GFP_NOFS);
			if (!r_page)
				return -ENOMEM;

			if (read_log_page(log, vbo,
					  (struct RECORD_PAGE_HDR **)&r_page,
					  &usa_error)) {
				/* Ignore any errors. */
				kfree(r_page);
				r_page = NULL;
				continue;
			}
		}

		if (is_client_area_valid(r_page, usa_error)) {
			info->valid_page = true;
			ra = Add2Ptr(r_page, le16_to_cpu(r_page->ra_off));
		}

check_result:
		/*
		 * If chkdsk was run then update the caller's
		 * values and return.
		 */
		if (r_page->rhdr.sign == NTFS_CHKD_SIGNATURE) {
			info->chkdsk_was_run = true;
			info->last_lsn = le64_to_cpu(r_page->rhdr.lsn);
			info->restart = true;
			info->r_page = r_page;
			return 0;
		}

		/*
		 * If we have a valid page then copy the values
		 * we need from it.
		 */
		if (info->valid_page) {
			info->last_lsn = le64_to_cpu(ra->current_lsn);
			info->restart = true;
			info->r_page = r_page;
			return 0;
		}
	}

	kfree(r_page);

	return 0;
}