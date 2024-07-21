int log_replay(struct ntfs_inode *ni, bool *initialized)
{
	int err;
	struct ntfs_sb_info *sbi = ni->mi.sbi;
	struct ntfs_log *log;

	struct restart_info rst_info, rst_info2;
	u64 rec_lsn, ra_lsn, checkpt_lsn = 0, rlsn = 0;
	struct ATTR_NAME_ENTRY *attr_names = NULL;
	struct ATTR_NAME_ENTRY *ane;
	struct RESTART_TABLE *dptbl = NULL;
	struct RESTART_TABLE *trtbl = NULL;
	const struct RESTART_TABLE *rt;
	struct RESTART_TABLE *oatbl = NULL;
	struct inode *inode;
	struct OpenAttr *oa;
	struct ntfs_inode *ni_oe;
	struct ATTRIB *attr = NULL;
	u64 size, vcn, undo_next_lsn;
	CLST rno, lcn, lcn0, len0, clen;
	void *data;
	struct NTFS_RESTART *rst = NULL;
	struct lcb *lcb = NULL;
	struct OPEN_ATTR_ENRTY *oe;
	struct TRANSACTION_ENTRY *tr;
	struct DIR_PAGE_ENTRY *dp;
	u32 i, bytes_per_attr_entry;
	u32 l_size = ni->vfs_inode.i_size;
	u32 orig_file_size = l_size;
	u32 page_size, vbo, tail, off, dlen;
	u32 saved_len, rec_len, transact_id;
	bool use_second_page;
	struct RESTART_AREA *ra2, *ra = NULL;
	struct CLIENT_REC *ca, *cr;
	__le16 client;
	struct RESTART_HDR *rh;
	const struct LFS_RECORD_HDR *frh;
	const struct LOG_REC_HDR *lrh;
	bool is_mapped;
	bool is_ro = sb_rdonly(sbi->sb);
	u64 t64;
	u16 t16;
	u32 t32;

	/* Get the size of page. NOTE: To replay we can use default page. */
#if PAGE_SIZE >= DefaultLogPageSize && PAGE_SIZE <= DefaultLogPageSize * 2
	page_size = norm_file_page(PAGE_SIZE, &l_size, true);
#else
	page_size = norm_file_page(PAGE_SIZE, &l_size, false);
#endif
	if (!page_size)
		return -EINVAL;

	log = kzalloc(sizeof(struct ntfs_log), GFP_NOFS);
	if (!log)
		return -ENOMEM;

	log->ni = ni;
	log->l_size = l_size;
	log->one_page_buf = kmalloc(page_size, GFP_NOFS);

	if (!log->one_page_buf) {
		err = -ENOMEM;
		goto out;
	}

	log->page_size = page_size;
	log->page_mask = page_size - 1;
	log->page_bits = blksize_bits(page_size);

	/* Look for a restart area on the disk. */
	err = log_read_rst(log, l_size, true, &rst_info);
	if (err)
		goto out;

	/* remember 'initialized' */
	*initialized = rst_info.initialized;

	if (!rst_info.restart) {
		if (rst_info.initialized) {
			/* No restart area but the file is not initialized. */
			err = -EINVAL;
			goto out;
		}

		log_init_pg_hdr(log, page_size, page_size, 1, 1);
		log_create(log, l_size, 0, get_random_int(), false, false);

		log->ra = ra;

		ra = log_create_ra(log);
		if (!ra) {
			err = -ENOMEM;
			goto out;
		}
		log->ra = ra;
		log->init_ra = true;

		goto process_log;
	}

	/*
	 * If the restart offset above wasn't zero then we won't
	 * look for a second restart.
	 */
	if (rst_info.vbo)
		goto check_restart_area;

	err = log_read_rst(log, l_size, false, &rst_info2);

	/* Determine which restart area to use. */
	if (!rst_info2.restart || rst_info2.last_lsn <= rst_info.last_lsn)
		goto use_first_page;

	use_second_page = true;

	if (rst_info.chkdsk_was_run && page_size != rst_info.vbo) {
		struct RECORD_PAGE_HDR *sp = NULL;
		bool usa_error;

		if (!read_log_page(log, page_size, &sp, &usa_error) &&
		    sp->rhdr.sign == NTFS_CHKD_SIGNATURE) {
			use_second_page = false;
		}
		kfree(sp);
	}

	if (use_second_page) {
		kfree(rst_info.r_page);
		memcpy(&rst_info, &rst_info2, sizeof(struct restart_info));
		rst_info2.r_page = NULL;
	}

use_first_page:
	kfree(rst_info2.r_page);

check_restart_area:
	/*
	 * If the restart area is at offset 0, we want
	 * to write the second restart area first.
	 */
	log->init_ra = !!rst_info.vbo;

	/* If we have a valid page then grab a pointer to the restart area. */
	ra2 = rst_info.valid_page
		      ? Add2Ptr(rst_info.r_page,
				le16_to_cpu(rst_info.r_page->ra_off))
		      : NULL;

	if (rst_info.chkdsk_was_run ||
	    (ra2 && ra2->client_idx[1] == LFS_NO_CLIENT_LE)) {
		bool wrapped = false;
		bool use_multi_page = false;
		u32 open_log_count;

		/* Do some checks based on whether we have a valid log page. */
		if (!rst_info.valid_page) {
			open_log_count = get_random_int();
			goto init_log_instance;
		}
		open_log_count = le32_to_cpu(ra2->open_log_count);

		/*
		 * If the restart page size isn't changing then we want to
		 * check how much work we need to do.
		 */
		if (page_size != le32_to_cpu(rst_info.r_page->sys_page_size))
			goto init_log_instance;

init_log_instance:
		log_init_pg_hdr(log, page_size, page_size, 1, 1);

		log_create(log, l_size, rst_info.last_lsn, open_log_count,
			   wrapped, use_multi_page);

		ra = log_create_ra(log);
		if (!ra) {
			err = -ENOMEM;
			goto out;
		}
		log->ra = ra;

		/* Put the restart areas and initialize
		 * the log file as required.
		 */
		goto process_log;
	}

	if (!ra2) {
		err = -EINVAL;
		goto out;
	}

	/*
	 * If the log page or the system page sizes have changed, we can't
	 * use the log file. We must use the system page size instead of the
	 * default size if there is not a clean shutdown.
	 */
	t32 = le32_to_cpu(rst_info.r_page->sys_page_size);
	if (page_size != t32) {
		l_size = orig_file_size;
		page_size =
			norm_file_page(t32, &l_size, t32 == DefaultLogPageSize);
	}

	if (page_size != t32 ||
	    page_size != le32_to_cpu(rst_info.r_page->page_size)) {
		err = -EINVAL;
		goto out;
	}

	/* If the file size has shrunk then we won't mount it. */
	if (l_size < le64_to_cpu(ra2->l_size)) {
		err = -EINVAL;
		goto out;
	}

	log_init_pg_hdr(log, page_size, page_size,
			le16_to_cpu(rst_info.r_page->major_ver),
			le16_to_cpu(rst_info.r_page->minor_ver));

	log->l_size = le64_to_cpu(ra2->l_size);
	log->seq_num_bits = le32_to_cpu(ra2->seq_num_bits);
	log->file_data_bits = sizeof(u64) * 8 - log->seq_num_bits;
	log->seq_num_mask = (8 << log->file_data_bits) - 1;
	log->last_lsn = le64_to_cpu(ra2->current_lsn);
	log->seq_num = log->last_lsn >> log->file_data_bits;
	log->ra_off = le16_to_cpu(rst_info.r_page->ra_off);
	log->restart_size = log->sys_page_size - log->ra_off;
	log->record_header_len = le16_to_cpu(ra2->rec_hdr_len);
	log->ra_size = le16_to_cpu(ra2->ra_len);
	log->data_off = le16_to_cpu(ra2->data_off);
	log->data_size = log->page_size - log->data_off;
	log->reserved = log->data_size - log->record_header_len;

	vbo = lsn_to_vbo(log, log->last_lsn);

	if (vbo < log->first_page) {
		/* This is a pseudo lsn. */
		log->l_flags |= NTFSLOG_NO_LAST_LSN;
		log->next_page = log->first_page;
		goto find_oldest;
	}

	/* Find the end of this log record. */
	off = final_log_off(log, log->last_lsn,
			    le32_to_cpu(ra2->last_lsn_data_len));

	/* If we wrapped the file then increment the sequence number. */
	if (off <= vbo) {
		log->seq_num += 1;
		log->l_flags |= NTFSLOG_WRAPPED;
	}

	/* Now compute the next log page to use. */
	vbo &= ~log->sys_page_mask;
	tail = log->page_size - (off & log->page_mask) - 1;

	/*
	 *If we can fit another log record on the page,
	 * move back a page the log file.
	 */
	if (tail >= log->record_header_len) {
		log->l_flags |= NTFSLOG_REUSE_TAIL;
		log->next_page = vbo;
	} else {
		log->next_page = next_page_off(log, vbo);
	}

find_oldest:
	/*
	 * Find the oldest client lsn. Use the last
	 * flushed lsn as a starting point.
	 */
	log->oldest_lsn = log->last_lsn;
	oldest_client_lsn(Add2Ptr(ra2, le16_to_cpu(ra2->client_off)),
			  ra2->client_idx[1], &log->oldest_lsn);
	log->oldest_lsn_off = lsn_to_vbo(log, log->oldest_lsn);

	if (log->oldest_lsn_off < log->first_page)
		log->l_flags |= NTFSLOG_NO_OLDEST_LSN;

	if (!(ra2->flags & RESTART_SINGLE_PAGE_IO))
		log->l_flags |= NTFSLOG_WRAPPED | NTFSLOG_MULTIPLE_PAGE_IO;

	log->current_openlog_count = le32_to_cpu(ra2->open_log_count);
	log->total_avail_pages = log->l_size - log->first_page;
	log->total_avail = log->total_avail_pages >> log->page_bits;
	log->max_current_avail = log->total_avail * log->reserved;
	log->total_avail = log->total_avail * log->data_size;

	log->current_avail = current_log_avail(log);

	ra = kzalloc(log->restart_size, GFP_NOFS);
	if (!ra) {
		err = -ENOMEM;
		goto out;
	}
	log->ra = ra;

	t16 = le16_to_cpu(ra2->client_off);
	if (t16 == offsetof(struct RESTART_AREA, clients)) {
		memcpy(ra, ra2, log->ra_size);
	} else {
		memcpy(ra, ra2, offsetof(struct RESTART_AREA, clients));
		memcpy(ra->clients, Add2Ptr(ra2, t16),
		       le16_to_cpu(ra2->ra_len) - t16);

		log->current_openlog_count = get_random_int();
		ra->open_log_count = cpu_to_le32(log->current_openlog_count);
		log->ra_size = offsetof(struct RESTART_AREA, clients) +
			       sizeof(struct CLIENT_REC);
		ra->client_off =
			cpu_to_le16(offsetof(struct RESTART_AREA, clients));
		ra->ra_len = cpu_to_le16(log->ra_size);
	}

	le32_add_cpu(&ra->open_log_count, 1);

	/* Now we need to walk through looking for the last lsn. */
	err = last_log_lsn(log);
	if (err)
		goto out;

	log->current_avail = current_log_avail(log);

	/* Remember which restart area to write first. */
	log->init_ra = rst_info.vbo;

process_log:
	/* 1.0, 1.1, 2.0 log->major_ver/minor_ver - short values. */
	switch ((log->major_ver << 16) + log->minor_ver) {
	case 0x10000:
	case 0x10001:
	case 0x20000:
		break;
	default:
		ntfs_warn(sbi->sb, "\x24LogFile version %d.%d is not supported",
			  log->major_ver, log->minor_ver);
		err = -EOPNOTSUPP;
		log->set_dirty = true;
		goto out;
	}

	/* One client "NTFS" per logfile. */
	ca = Add2Ptr(ra, le16_to_cpu(ra->client_off));

	for (client = ra->client_idx[1];; client = cr->next_client) {
		if (client == LFS_NO_CLIENT_LE) {
			/* Insert "NTFS" client LogFile. */
			client = ra->client_idx[0];
			if (client == LFS_NO_CLIENT_LE) {
				err = -EINVAL;
				goto out;
			}

			t16 = le16_to_cpu(client);
			cr = ca + t16;

			remove_client(ca, cr, &ra->client_idx[0]);

			cr->restart_lsn = 0;
			cr->oldest_lsn = cpu_to_le64(log->oldest_lsn);
			cr->name_bytes = cpu_to_le32(8);
			cr->name[0] = cpu_to_le16('N');
			cr->name[1] = cpu_to_le16('T');
			cr->name[2] = cpu_to_le16('F');
			cr->name[3] = cpu_to_le16('S');

			add_client(ca, t16, &ra->client_idx[1]);
			break;
		}

		cr = ca + le16_to_cpu(client);

		if (cpu_to_le32(8) == cr->name_bytes &&
		    cpu_to_le16('N') == cr->name[0] &&
		    cpu_to_le16('T') == cr->name[1] &&
		    cpu_to_le16('F') == cr->name[2] &&
		    cpu_to_le16('S') == cr->name[3])
			break;
	}

	/* Update the client handle with the client block information. */
	log->client_id.seq_num = cr->seq_num;
	log->client_id.client_idx = client;

	err = read_rst_area(log, &rst, &ra_lsn);
	if (err)
		goto out;

	if (!rst)
		goto out;

	bytes_per_attr_entry = !rst->major_ver ? 0x2C : 0x28;

	checkpt_lsn = le64_to_cpu(rst->check_point_start);
	if (!checkpt_lsn)
		checkpt_lsn = ra_lsn;

	/* Allocate and Read the Transaction Table. */
	if (!rst->transact_table_len)
		goto check_dirty_page_table;

	t64 = le64_to_cpu(rst->transact_table_lsn);
	err = read_log_rec_lcb(log, t64, lcb_ctx_prev, &lcb);
	if (err)
		goto out;

	lrh = lcb->log_rec;
	frh = lcb->lrh;
	rec_len = le32_to_cpu(frh->client_data_len);

	if (!check_log_rec(lrh, rec_len, le32_to_cpu(frh->transact_id),
			   bytes_per_attr_entry)) {
		err = -EINVAL;
		goto out;
	}

	t16 = le16_to_cpu(lrh->redo_off);

	rt = Add2Ptr(lrh, t16);
	t32 = rec_len - t16;

	/* Now check that this is a valid restart table. */
	if (!check_rstbl(rt, t32)) {
		err = -EINVAL;
		goto out;
	}

	trtbl = kmemdup(rt, t32, GFP_NOFS);
	if (!trtbl) {
		err = -ENOMEM;
		goto out;
	}

	lcb_put(lcb);
	lcb = NULL;

check_dirty_page_table:
	/* The next record back should be the Dirty Pages Table. */
	if (!rst->dirty_pages_len)
		goto check_attribute_names;

	t64 = le64_to_cpu(rst->dirty_pages_table_lsn);
	err = read_log_rec_lcb(log, t64, lcb_ctx_prev, &lcb);
	if (err)
		goto out;

	lrh = lcb->log_rec;
	frh = lcb->lrh;
	rec_len = le32_to_cpu(frh->client_data_len);

	if (!check_log_rec(lrh, rec_len, le32_to_cpu(frh->transact_id),
			   bytes_per_attr_entry)) {
		err = -EINVAL;
		goto out;
	}

	t16 = le16_to_cpu(lrh->redo_off);

	rt = Add2Ptr(lrh, t16);
	t32 = rec_len - t16;

	/* Now check that this is a valid restart table. */
	if (!check_rstbl(rt, t32)) {
		err = -EINVAL;
		goto out;
	}

	dptbl = kmemdup(rt, t32, GFP_NOFS);
	if (!dptbl) {
		err = -ENOMEM;
		goto out;
	}

	/* Convert Ra version '0' into version '1'. */
	if (rst->major_ver)
		goto end_conv_1;

	dp = NULL;
	while ((dp = enum_rstbl(dptbl, dp))) {
		struct DIR_PAGE_ENTRY_32 *dp0 = (struct DIR_PAGE_ENTRY_32 *)dp;
		// NOTE: Danger. Check for of boundary.
		memmove(&dp->vcn, &dp0->vcn_low,
			2 * sizeof(u64) +
				le32_to_cpu(dp->lcns_follow) * sizeof(u64));
	}

end_conv_1:
	lcb_put(lcb);
	lcb = NULL;

	/*
	 * Go through the table and remove the duplicates,
	 * remembering the oldest lsn values.
	 */
	if (sbi->cluster_size <= log->page_size)
		goto trace_dp_table;

	dp = NULL;
	while ((dp = enum_rstbl(dptbl, dp))) {
		struct DIR_PAGE_ENTRY *next = dp;

		while ((next = enum_rstbl(dptbl, next))) {
			if (next->target_attr == dp->target_attr &&
			    next->vcn == dp->vcn) {
				if (le64_to_cpu(next->oldest_lsn) <
				    le64_to_cpu(dp->oldest_lsn)) {
					dp->oldest_lsn = next->oldest_lsn;
				}

				free_rsttbl_idx(dptbl, PtrOffset(dptbl, next));
			}
		}
	}
trace_dp_table:
check_attribute_names:
	/* The next record should be the Attribute Names. */
	if (!rst->attr_names_len)
		goto check_attr_table;

	t64 = le64_to_cpu(rst->attr_names_lsn);
	err = read_log_rec_lcb(log, t64, lcb_ctx_prev, &lcb);
	if (err)
		goto out;

	lrh = lcb->log_rec;
	frh = lcb->lrh;
	rec_len = le32_to_cpu(frh->client_data_len);

	if (!check_log_rec(lrh, rec_len, le32_to_cpu(frh->transact_id),
			   bytes_per_attr_entry)) {
		err = -EINVAL;
		goto out;
	}

	t32 = lrh_length(lrh);
	rec_len -= t32;

	attr_names = kmemdup(Add2Ptr(lrh, t32), rec_len, GFP_NOFS);

	lcb_put(lcb);
	lcb = NULL;

check_attr_table:
	/* The next record should be the attribute Table. */
	if (!rst->open_attr_len)
		goto check_attribute_names2;

	t64 = le64_to_cpu(rst->open_attr_table_lsn);
	err = read_log_rec_lcb(log, t64, lcb_ctx_prev, &lcb);
	if (err)
		goto out;

	lrh = lcb->log_rec;
	frh = lcb->lrh;
	rec_len = le32_to_cpu(frh->client_data_len);

	if (!check_log_rec(lrh, rec_len, le32_to_cpu(frh->transact_id),
			   bytes_per_attr_entry)) {
		err = -EINVAL;
		goto out;
	}

	t16 = le16_to_cpu(lrh->redo_off);

	rt = Add2Ptr(lrh, t16);
	t32 = rec_len - t16;

	if (!check_rstbl(rt, t32)) {
		err = -EINVAL;
		goto out;
	}

	oatbl = kmemdup(rt, t32, GFP_NOFS);
	if (!oatbl) {
		err = -ENOMEM;
		goto out;
	}

	log->open_attr_tbl = oatbl;

	/* Clear all of the Attr pointers. */
	oe = NULL;
	while ((oe = enum_rstbl(oatbl, oe))) {
		if (!rst->major_ver) {
			struct OPEN_ATTR_ENRTY_32 oe0;

			/* Really 'oe' points to OPEN_ATTR_ENRTY_32. */
			memcpy(&oe0, oe, SIZEOF_OPENATTRIBUTEENTRY0);

			oe->bytes_per_index = oe0.bytes_per_index;
			oe->type = oe0.type;
			oe->is_dirty_pages = oe0.is_dirty_pages;
			oe->name_len = 0;
			oe->ref = oe0.ref;
			oe->open_record_lsn = oe0.open_record_lsn;
		}

		oe->is_attr_name = 0;
		oe->ptr = NULL;
	}

	lcb_put(lcb);
	lcb = NULL;

check_attribute_names2:
	if (!rst->attr_names_len)
		goto trace_attribute_table;

	ane = attr_names;
	if (!oatbl)
		goto trace_attribute_table;
	while (ane->off) {
		/* TODO: Clear table on exit! */
		oe = Add2Ptr(oatbl, le16_to_cpu(ane->off));
		t16 = le16_to_cpu(ane->name_bytes);
		oe->name_len = t16 / sizeof(short);
		oe->ptr = ane->name;
		oe->is_attr_name = 2;
		ane = Add2Ptr(ane, sizeof(struct ATTR_NAME_ENTRY) + t16);
	}

trace_attribute_table:
	/*
	 * If the checkpt_lsn is zero, then this is a freshly
	 * formatted disk and we have no work to do.
	 */
	if (!checkpt_lsn) {
		err = 0;
		goto out;
	}

	if (!oatbl) {
		oatbl = init_rsttbl(bytes_per_attr_entry, 8);
		if (!oatbl) {
			err = -ENOMEM;
			goto out;
		}
	}

	log->open_attr_tbl = oatbl;

	/* Start the analysis pass from the Checkpoint lsn. */
	rec_lsn = checkpt_lsn;

	/* Read the first lsn. */
	err = read_log_rec_lcb(log, checkpt_lsn, lcb_ctx_next, &lcb);
	if (err)
		goto out;

	/* Loop to read all subsequent records to the end of the log file. */
next_log_record_analyze:
	err = read_next_log_rec(log, lcb, &rec_lsn);
	if (err)
		goto out;

	if (!rec_lsn)
		goto end_log_records_enumerate;

	frh = lcb->lrh;
	transact_id = le32_to_cpu(frh->transact_id);
	rec_len = le32_to_cpu(frh->client_data_len);
	lrh = lcb->log_rec;

	if (!check_log_rec(lrh, rec_len, transact_id, bytes_per_attr_entry)) {
		err = -EINVAL;
		goto out;
	}

	/*
	 * The first lsn after the previous lsn remembered
	 * the checkpoint is the first candidate for the rlsn.
	 */
	if (!rlsn)
		rlsn = rec_lsn;

	if (LfsClientRecord != frh->record_type)
		goto next_log_record_analyze;

	/*
	 * Now update the Transaction Table for this transaction. If there
	 * is no entry present or it is unallocated we allocate the entry.
	 */
	if (!trtbl) {
		trtbl = init_rsttbl(sizeof(struct TRANSACTION_ENTRY),
				    INITIAL_NUMBER_TRANSACTIONS);
		if (!trtbl) {
			err = -ENOMEM;
			goto out;
		}
	}

	tr = Add2Ptr(trtbl, transact_id);

	if (transact_id >= bytes_per_rt(trtbl) ||
	    tr->next != RESTART_ENTRY_ALLOCATED_LE) {
		tr = alloc_rsttbl_from_idx(&trtbl, transact_id);
		if (!tr) {
			err = -ENOMEM;
			goto out;
		}
		tr->transact_state = TransactionActive;
		tr->first_lsn = cpu_to_le64(rec_lsn);
	}

	tr->prev_lsn = tr->undo_next_lsn = cpu_to_le64(rec_lsn);

	/*
	 * If this is a compensation log record, then change
	 * the undo_next_lsn to be the undo_next_lsn of this record.
	 */
	if (lrh->undo_op == cpu_to_le16(CompensationLogRecord))
		tr->undo_next_lsn = frh->client_undo_next_lsn;

	/* Dispatch to handle log record depending on type. */
	switch (le16_to_cpu(lrh->redo_op)) {
	case InitializeFileRecordSegment:
	case DeallocateFileRecordSegment:
	case WriteEndOfFileRecordSegment:
	case CreateAttribute:
	case DeleteAttribute:
	case UpdateResidentValue:
	case UpdateNonresidentValue:
	case UpdateMappingPairs:
	case SetNewAttributeSizes:
	case AddIndexEntryRoot:
	case DeleteIndexEntryRoot:
	case AddIndexEntryAllocation:
	case DeleteIndexEntryAllocation:
	case WriteEndOfIndexBuffer:
	case SetIndexEntryVcnRoot:
	case SetIndexEntryVcnAllocation:
	case UpdateFileNameRoot:
	case UpdateFileNameAllocation:
	case SetBitsInNonresidentBitMap:
	case ClearBitsInNonresidentBitMap:
	case UpdateRecordDataRoot:
	case UpdateRecordDataAllocation:
	case ZeroEndOfFileRecord:
		t16 = le16_to_cpu(lrh->target_attr);
		t64 = le64_to_cpu(lrh->target_vcn);
		dp = find_dp(dptbl, t16, t64);

		if (dp)
			goto copy_lcns;

		/*
		 * Calculate the number of clusters per page the system
		 * which wrote the checkpoint, possibly creating the table.
		 */
		if (dptbl) {
			t32 = (le16_to_cpu(dptbl->size) -
			       sizeof(struct DIR_PAGE_ENTRY)) /
			      sizeof(u64);
		} else {
			t32 = log->clst_per_page;
			kfree(dptbl);
			dptbl = init_rsttbl(struct_size(dp, page_lcns, t32),
					    32);
			if (!dptbl) {
				err = -ENOMEM;
				goto out;
			}
		}

		dp = alloc_rsttbl_idx(&dptbl);
		if (!dp) {
			err = -ENOMEM;
			goto out;
		}
		dp->target_attr = cpu_to_le32(t16);
		dp->transfer_len = cpu_to_le32(t32 << sbi->cluster_bits);
		dp->lcns_follow = cpu_to_le32(t32);
		dp->vcn = cpu_to_le64(t64 & ~((u64)t32 - 1));
		dp->oldest_lsn = cpu_to_le64(rec_lsn);

copy_lcns:
		/*
		 * Copy the Lcns from the log record into the Dirty Page Entry.
		 * TODO: For different page size support, must somehow make
		 * whole routine a loop, case Lcns do not fit below.
		 */
		t16 = le16_to_cpu(lrh->lcns_follow);
		for (i = 0; i < t16; i++) {
			size_t j = (size_t)(le64_to_cpu(lrh->target_vcn) -
					    le64_to_cpu(dp->vcn));
			dp->page_lcns[j + i] = lrh->page_lcns[i];
		}

		goto next_log_record_analyze;

	case DeleteDirtyClusters: {
		u32 range_count =
			le16_to_cpu(lrh->redo_len) / sizeof(struct LCN_RANGE);
		const struct LCN_RANGE *r =
			Add2Ptr(lrh, le16_to_cpu(lrh->redo_off));

		/* Loop through all of the Lcn ranges this log record. */
		for (i = 0; i < range_count; i++, r++) {
			u64 lcn0 = le64_to_cpu(r->lcn);
			u64 lcn_e = lcn0 + le64_to_cpu(r->len) - 1;

			dp = NULL;
			while ((dp = enum_rstbl(dptbl, dp))) {
				u32 j;

				t32 = le32_to_cpu(dp->lcns_follow);
				for (j = 0; j < t32; j++) {
					t64 = le64_to_cpu(dp->page_lcns[j]);
					if (t64 >= lcn0 && t64 <= lcn_e)
						dp->page_lcns[j] = 0;
				}
			}
		}
		goto next_log_record_analyze;
		;
	}

	case OpenNonresidentAttribute:
		t16 = le16_to_cpu(lrh->target_attr);
		if (t16 >= bytes_per_rt(oatbl)) {
			/*
			 * Compute how big the table needs to be.
			 * Add 10 extra entries for some cushion.
			 */
			u32 new_e = t16 / le16_to_cpu(oatbl->size);

			new_e += 10 - le16_to_cpu(oatbl->used);

			oatbl = extend_rsttbl(oatbl, new_e, ~0u);
			log->open_attr_tbl = oatbl;
			if (!oatbl) {
				err = -ENOMEM;
				goto out;
			}
		}

		/* Point to the entry being opened. */
		oe = alloc_rsttbl_from_idx(&oatbl, t16);
		log->open_attr_tbl = oatbl;
		if (!oe) {
			err = -ENOMEM;
			goto out;
		}

		/* Initialize this entry from the log record. */
		t16 = le16_to_cpu(lrh->redo_off);
		if (!rst->major_ver) {
			/* Convert version '0' into version '1'. */
			struct OPEN_ATTR_ENRTY_32 *oe0 = Add2Ptr(lrh, t16);

			oe->bytes_per_index = oe0->bytes_per_index;
			oe->type = oe0->type;
			oe->is_dirty_pages = oe0->is_dirty_pages;
			oe->name_len = 0; //oe0.name_len;
			oe->ref = oe0->ref;
			oe->open_record_lsn = oe0->open_record_lsn;
		} else {
			memcpy(oe, Add2Ptr(lrh, t16), bytes_per_attr_entry);
		}

		t16 = le16_to_cpu(lrh->undo_len);
		if (t16) {
			oe->ptr = kmalloc(t16, GFP_NOFS);
			if (!oe->ptr) {
				err = -ENOMEM;
				goto out;
			}
			oe->name_len = t16 / sizeof(short);
			memcpy(oe->ptr,
			       Add2Ptr(lrh, le16_to_cpu(lrh->undo_off)), t16);
			oe->is_attr_name = 1;
		} else {
			oe->ptr = NULL;
			oe->is_attr_name = 0;
		}

		goto next_log_record_analyze;

	case HotFix:
		t16 = le16_to_cpu(lrh->target_attr);
		t64 = le64_to_cpu(lrh->target_vcn);
		dp = find_dp(dptbl, t16, t64);
		if (dp) {
			size_t j = le64_to_cpu(lrh->target_vcn) -
				   le64_to_cpu(dp->vcn);
			if (dp->page_lcns[j])
				dp->page_lcns[j] = lrh->page_lcns[0];
		}
		goto next_log_record_analyze;

	case EndTopLevelAction:
		tr = Add2Ptr(trtbl, transact_id);
		tr->prev_lsn = cpu_to_le64(rec_lsn);
		tr->undo_next_lsn = frh->client_undo_next_lsn;
		goto next_log_record_analyze;

	case PrepareTransaction:
		tr = Add2Ptr(trtbl, transact_id);
		tr->transact_state = TransactionPrepared;
		goto next_log_record_analyze;

	case CommitTransaction:
		tr = Add2Ptr(trtbl, transact_id);
		tr->transact_state = TransactionCommitted;
		goto next_log_record_analyze;

	case ForgetTransaction:
		free_rsttbl_idx(trtbl, transact_id);
		goto next_log_record_analyze;

	case Noop:
	case OpenAttributeTableDump:
	case AttributeNamesDump:
	case DirtyPageTableDump:
	case TransactionTableDump:
		/* The following cases require no action the Analysis Pass. */
		goto next_log_record_analyze;

	default:
		/*
		 * All codes will be explicitly handled.
		 * If we see a code we do not expect, then we are trouble.
		 */
		goto next_log_record_analyze;
	}

end_log_records_enumerate:
	lcb_put(lcb);
	lcb = NULL;

	/*
	 * Scan the Dirty Page Table and Transaction Table for
	 * the lowest lsn, and return it as the Redo lsn.
	 */
	dp = NULL;
	while ((dp = enum_rstbl(dptbl, dp))) {
		t64 = le64_to_cpu(dp->oldest_lsn);
		if (t64 && t64 < rlsn)
			rlsn = t64;
	}

	tr = NULL;
	while ((tr = enum_rstbl(trtbl, tr))) {
		t64 = le64_to_cpu(tr->first_lsn);
		if (t64 && t64 < rlsn)
			rlsn = t64;
	}

	/*
	 * Only proceed if the Dirty Page Table or Transaction
	 * table are not empty.
	 */
	if ((!dptbl || !dptbl->total) && (!trtbl || !trtbl->total))
		goto end_reply;

	sbi->flags |= NTFS_FLAGS_NEED_REPLAY;
	if (is_ro)
		goto out;

	/* Reopen all of the attributes with dirty pages. */
	oe = NULL;
next_open_attribute:

	oe = enum_rstbl(oatbl, oe);
	if (!oe) {
		err = 0;
		dp = NULL;
		goto next_dirty_page;
	}

	oa = kzalloc(sizeof(struct OpenAttr), GFP_NOFS);
	if (!oa) {
		err = -ENOMEM;
		goto out;
	}

	inode = ntfs_iget5(sbi->sb, &oe->ref, NULL);
	if (IS_ERR(inode))
		goto fake_attr;

	if (is_bad_inode(inode)) {
		iput(inode);
fake_attr:
		if (oa->ni) {
			iput(&oa->ni->vfs_inode);
			oa->ni = NULL;
		}

		attr = attr_create_nonres_log(sbi, oe->type, 0, oe->ptr,
					      oe->name_len, 0);
		if (!attr) {
			kfree(oa);
			err = -ENOMEM;
			goto out;
		}
		oa->attr = attr;
		oa->run1 = &oa->run0;
		goto final_oe;
	}

	ni_oe = ntfs_i(inode);
	oa->ni = ni_oe;

	attr = ni_find_attr(ni_oe, NULL, NULL, oe->type, oe->ptr, oe->name_len,
			    NULL, NULL);

	if (!attr)
		goto fake_attr;

	t32 = le32_to_cpu(attr->size);
	oa->attr = kmemdup(attr, t32, GFP_NOFS);
	if (!oa->attr)
		goto fake_attr;

	if (!S_ISDIR(inode->i_mode)) {
		if (attr->type == ATTR_DATA && !attr->name_len) {
			oa->run1 = &ni_oe->file.run;
			goto final_oe;
		}
	} else {
		if (attr->type == ATTR_ALLOC &&
		    attr->name_len == ARRAY_SIZE(I30_NAME) &&
		    !memcmp(attr_name(attr), I30_NAME, sizeof(I30_NAME))) {
			oa->run1 = &ni_oe->dir.alloc_run;
			goto final_oe;
		}
	}

	if (attr->non_res) {
		u16 roff = le16_to_cpu(attr->nres.run_off);
		CLST svcn = le64_to_cpu(attr->nres.svcn);

		err = run_unpack(&oa->run0, sbi, inode->i_ino, svcn,
				 le64_to_cpu(attr->nres.evcn), svcn,
				 Add2Ptr(attr, roff), t32 - roff);
		if (err < 0) {
			kfree(oa->attr);
			oa->attr = NULL;
			goto fake_attr;
		}
		err = 0;
	}
	oa->run1 = &oa->run0;
	attr = oa->attr;

final_oe:
	if (oe->is_attr_name == 1)
		kfree(oe->ptr);
	oe->is_attr_name = 0;
	oe->ptr = oa;
	oe->name_len = attr->name_len;

	goto next_open_attribute;

	/*
	 * Now loop through the dirty page table to extract all of the Vcn/Lcn.
	 * Mapping that we have, and insert it into the appropriate run.
	 */
next_dirty_page:
	dp = enum_rstbl(dptbl, dp);
	if (!dp)
		goto do_redo_1;

	oe = Add2Ptr(oatbl, le32_to_cpu(dp->target_attr));

	if (oe->next != RESTART_ENTRY_ALLOCATED_LE)
		goto next_dirty_page;

	oa = oe->ptr;
	if (!oa)
		goto next_dirty_page;

	i = -1;
next_dirty_page_vcn:
	i += 1;
	if (i >= le32_to_cpu(dp->lcns_follow))
		goto next_dirty_page;

	vcn = le64_to_cpu(dp->vcn) + i;
	size = (vcn + 1) << sbi->cluster_bits;

	if (!dp->page_lcns[i])
		goto next_dirty_page_vcn;

	rno = ino_get(&oe->ref);
	if (rno <= MFT_REC_MIRR &&
	    size < (MFT_REC_VOL + 1) * sbi->record_size &&
	    oe->type == ATTR_DATA) {
		goto next_dirty_page_vcn;
	}

	lcn = le64_to_cpu(dp->page_lcns[i]);

	if ((!run_lookup_entry(oa->run1, vcn, &lcn0, &len0, NULL) ||
	     lcn0 != lcn) &&
	    !run_add_entry(oa->run1, vcn, lcn, 1, false)) {
		err = -ENOMEM;
		goto out;
	}
	attr = oa->attr;
	t64 = le64_to_cpu(attr->nres.alloc_size);
	if (size > t64) {
		attr->nres.valid_size = attr->nres.data_size =
			attr->nres.alloc_size = cpu_to_le64(size);
	}
	goto next_dirty_page_vcn;

do_redo_1:
	/*
	 * Perform the Redo Pass, to restore all of the dirty pages to the same
	 * contents that they had immediately before the crash. If the dirty
	 * page table is empty, then we can skip the entire Redo Pass.
	 */
	if (!dptbl || !dptbl->total)
		goto do_undo_action;

	rec_lsn = rlsn;

	/*
	 * Read the record at the Redo lsn, before falling
	 * into common code to handle each record.
	 */
	err = read_log_rec_lcb(log, rlsn, lcb_ctx_next, &lcb);
	if (err)
		goto out;

	/*
	 * Now loop to read all of our log records forwards, until
	 * we hit the end of the file, cleaning up at the end.
	 */
do_action_next:
	frh = lcb->lrh;

	if (LfsClientRecord != frh->record_type)
		goto read_next_log_do_action;

	transact_id = le32_to_cpu(frh->transact_id);
	rec_len = le32_to_cpu(frh->client_data_len);
	lrh = lcb->log_rec;

	if (!check_log_rec(lrh, rec_len, transact_id, bytes_per_attr_entry)) {
		err = -EINVAL;
		goto out;
	}

	/* Ignore log records that do not update pages. */
	if (lrh->lcns_follow)
		goto find_dirty_page;

	goto read_next_log_do_action;

find_dirty_page:
	t16 = le16_to_cpu(lrh->target_attr);
	t64 = le64_to_cpu(lrh->target_vcn);
	dp = find_dp(dptbl, t16, t64);

	if (!dp)
		goto read_next_log_do_action;

	if (rec_lsn < le64_to_cpu(dp->oldest_lsn))
		goto read_next_log_do_action;

	t16 = le16_to_cpu(lrh->target_attr);
	if (t16 >= bytes_per_rt(oatbl)) {
		err = -EINVAL;
		goto out;
	}

	oe = Add2Ptr(oatbl, t16);

	if (oe->next != RESTART_ENTRY_ALLOCATED_LE) {
		err = -EINVAL;
		goto out;
	}

	oa = oe->ptr;

	if (!oa) {
		err = -EINVAL;
		goto out;
	}
	attr = oa->attr;

	vcn = le64_to_cpu(lrh->target_vcn);

	if (!run_lookup_entry(oa->run1, vcn, &lcn, NULL, NULL) ||
	    lcn == SPARSE_LCN) {
		goto read_next_log_do_action;
	}

	/* Point to the Redo data and get its length. */
	data = Add2Ptr(lrh, le16_to_cpu(lrh->redo_off));
	dlen = le16_to_cpu(lrh->redo_len);

	/* Shorten length by any Lcns which were deleted. */
	saved_len = dlen;

	for (i = le16_to_cpu(lrh->lcns_follow); i; i--) {
		size_t j;
		u32 alen, voff;

		voff = le16_to_cpu(lrh->record_off) +
		       le16_to_cpu(lrh->attr_off);
		voff += le16_to_cpu(lrh->cluster_off) << SECTOR_SHIFT;

		/* If the Vcn question is allocated, we can just get out. */
		j = le64_to_cpu(lrh->target_vcn) - le64_to_cpu(dp->vcn);
		if (dp->page_lcns[j + i - 1])
			break;

		if (!saved_len)
			saved_len = 1;

		/*
		 * Calculate the allocated space left relative to the
		 * log record Vcn, after removing this unallocated Vcn.
		 */
		alen = (i - 1) << sbi->cluster_bits;

		/*
		 * If the update described this log record goes beyond
		 * the allocated space, then we will have to reduce the length.
		 */
		if (voff >= alen)
			dlen = 0;
		else if (voff + dlen > alen)
			dlen = alen - voff;
	}

	/*
	 * If the resulting dlen from above is now zero,
	 * we can skip this log record.
	 */
	if (!dlen && saved_len)
		goto read_next_log_do_action;

	t16 = le16_to_cpu(lrh->redo_op);
	if (can_skip_action(t16))
		goto read_next_log_do_action;

	/* Apply the Redo operation a common routine. */
	err = do_action(log, oe, lrh, t16, data, dlen, rec_len, &rec_lsn);
	if (err)
		goto out;

	/* Keep reading and looping back until end of file. */
read_next_log_do_action:
	err = read_next_log_rec(log, lcb, &rec_lsn);
	if (!err && rec_lsn)
		goto do_action_next;

	lcb_put(lcb);
	lcb = NULL;

do_undo_action:
	/* Scan Transaction Table. */
	tr = NULL;
transaction_table_next:
	tr = enum_rstbl(trtbl, tr);
	if (!tr)
		goto undo_action_done;

	if (TransactionActive != tr->transact_state || !tr->undo_next_lsn) {
		free_rsttbl_idx(trtbl, PtrOffset(trtbl, tr));
		goto transaction_table_next;
	}

	log->transaction_id = PtrOffset(trtbl, tr);
	undo_next_lsn = le64_to_cpu(tr->undo_next_lsn);

	/*
	 * We only have to do anything if the transaction has
	 * something its undo_next_lsn field.
	 */
	if (!undo_next_lsn)
		goto commit_undo;

	/* Read the first record to be undone by this transaction. */
	err = read_log_rec_lcb(log, undo_next_lsn, lcb_ctx_undo_next, &lcb);
	if (err)
		goto out;

	/*
	 * Now loop to read all of our log records forwards,
	 * until we hit the end of the file, cleaning up at the end.
	 */
undo_action_next:

	lrh = lcb->log_rec;
	frh = lcb->lrh;
	transact_id = le32_to_cpu(frh->transact_id);
	rec_len = le32_to_cpu(frh->client_data_len);

	if (!check_log_rec(lrh, rec_len, transact_id, bytes_per_attr_entry)) {
		err = -EINVAL;
		goto out;
	}

	if (lrh->undo_op == cpu_to_le16(Noop))
		goto read_next_log_undo_action;

	oe = Add2Ptr(oatbl, le16_to_cpu(lrh->target_attr));
	oa = oe->ptr;

	t16 = le16_to_cpu(lrh->lcns_follow);
	if (!t16)
		goto add_allocated_vcns;

	is_mapped = run_lookup_entry(oa->run1, le64_to_cpu(lrh->target_vcn),
				     &lcn, &clen, NULL);

	/*
	 * If the mapping isn't already the table or the  mapping
	 * corresponds to a hole the mapping, we need to make sure
	 * there is no partial page already memory.
	 */
	if (is_mapped && lcn != SPARSE_LCN && clen >= t16)
		goto add_allocated_vcns;

	vcn = le64_to_cpu(lrh->target_vcn);
	vcn &= ~(log->clst_per_page - 1);

add_allocated_vcns:
	for (i = 0, vcn = le64_to_cpu(lrh->target_vcn),
	    size = (vcn + 1) << sbi->cluster_bits;
	     i < t16; i++, vcn += 1, size += sbi->cluster_size) {
		attr = oa->attr;
		if (!attr->non_res) {
			if (size > le32_to_cpu(attr->res.data_size))
				attr->res.data_size = cpu_to_le32(size);
		} else {
			if (size > le64_to_cpu(attr->nres.data_size))
				attr->nres.valid_size = attr->nres.data_size =
					attr->nres.alloc_size =
						cpu_to_le64(size);
		}
	}

	t16 = le16_to_cpu(lrh->undo_op);
	if (can_skip_action(t16))
		goto read_next_log_undo_action;

	/* Point to the Redo data and get its length. */
	data = Add2Ptr(lrh, le16_to_cpu(lrh->undo_off));
	dlen = le16_to_cpu(lrh->undo_len);

	/* It is time to apply the undo action. */
	err = do_action(log, oe, lrh, t16, data, dlen, rec_len, NULL);

read_next_log_undo_action:
	/*
	 * Keep reading and looping back until we have read the
	 * last record for this transaction.
	 */
	err = read_next_log_rec(log, lcb, &rec_lsn);
	if (err)
		goto out;

	if (rec_lsn)
		goto undo_action_next;

	lcb_put(lcb);
	lcb = NULL;

commit_undo:
	free_rsttbl_idx(trtbl, log->transaction_id);

	log->transaction_id = 0;

	goto transaction_table_next;

undo_action_done:

	ntfs_update_mftmirr(sbi, 0);

	sbi->flags &= ~NTFS_FLAGS_NEED_REPLAY;

end_reply:

	err = 0;
	if (is_ro)
		goto out;

	rh = kzalloc(log->page_size, GFP_NOFS);
	if (!rh) {
		err = -ENOMEM;
		goto out;
	}

	rh->rhdr.sign = NTFS_RSTR_SIGNATURE;
	rh->rhdr.fix_off = cpu_to_le16(offsetof(struct RESTART_HDR, fixups));
	t16 = (log->page_size >> SECTOR_SHIFT) + 1;
	rh->rhdr.fix_num = cpu_to_le16(t16);
	rh->sys_page_size = cpu_to_le32(log->page_size);
	rh->page_size = cpu_to_le32(log->page_size);

	t16 = ALIGN(offsetof(struct RESTART_HDR, fixups) + sizeof(short) * t16,
		    8);
	rh->ra_off = cpu_to_le16(t16);
	rh->minor_ver = cpu_to_le16(1); // 0x1A:
	rh->major_ver = cpu_to_le16(1); // 0x1C:

	ra2 = Add2Ptr(rh, t16);
	memcpy(ra2, ra, sizeof(struct RESTART_AREA));

	ra2->client_idx[0] = 0;
	ra2->client_idx[1] = LFS_NO_CLIENT_LE;
	ra2->flags = cpu_to_le16(2);

	le32_add_cpu(&ra2->open_log_count, 1);

	ntfs_fix_pre_write(&rh->rhdr, log->page_size);

	err = ntfs_sb_write_run(sbi, &ni->file.run, 0, rh, log->page_size, 0);
	if (!err)
		err = ntfs_sb_write_run(sbi, &log->ni->file.run, log->page_size,
					rh, log->page_size, 0);

	kfree(rh);
	if (err)
		goto out;

out:
	kfree(rst);
	if (lcb)
		lcb_put(lcb);

	/*
	 * Scan the Open Attribute Table to close all of
	 * the open attributes.
	 */
	oe = NULL;
	while ((oe = enum_rstbl(oatbl, oe))) {
		rno = ino_get(&oe->ref);

		if (oe->is_attr_name == 1) {
			kfree(oe->ptr);
			oe->ptr = NULL;
			continue;
		}

		if (oe->is_attr_name)
			continue;

		oa = oe->ptr;
		if (!oa)
			continue;

		run_close(&oa->run0);
		kfree(oa->attr);
		if (oa->ni)
			iput(&oa->ni->vfs_inode);
		kfree(oa);
	}

	kfree(trtbl);
	kfree(oatbl);
	kfree(dptbl);
	kfree(attr_names);
	kfree(rst_info.r_page);

	kfree(ra);
	kfree(log->one_page_buf);

	if (err)
		sbi->flags |= NTFS_FLAGS_NEED_REPLAY;

	if (err == -EROFS)
		err = 0;
	else if (log->set_dirty)
		ntfs_set_state(sbi, NTFS_DIRTY_ERROR);

	kfree(log);

	return err;
}