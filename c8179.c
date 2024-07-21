errcode_t quota_compare_and_update(quota_ctx_t qctx, enum quota_type qtype,
				   int *usage_inconsistent)
{
	struct quota_handle qh;
	struct scan_dquots_data scan_data;
	struct dquot *dq;
	dnode_t *n;
	dict_t *dict = qctx->quota_dict[qtype];
	errcode_t err = 0;

	if (!dict)
		goto out;

	err = quota_file_open(qctx, &qh, 0, qtype, -1, 0);
	if (err) {
		log_debug("Open quota file failed");
		goto out;
	}

	scan_data.quota_dict = qctx->quota_dict[qtype];
	scan_data.update_limits = 1;
	scan_data.update_usage = 0;
	scan_data.check_consistency = 1;
	scan_data.usage_is_inconsistent = 0;
	err = qh.qh_ops->scan_dquots(&qh, scan_dquots_callback, &scan_data);
	if (err) {
		log_debug("Error scanning dquots");
		goto out_close_qh;
	}

	for (n = dict_first(dict); n; n = dict_next(dict, n)) {
		dq = dnode_get(n);
		if (!dq)
			continue;
		if ((dq->dq_flags & DQF_SEEN) == 0) {
			fprintf(stderr, "[QUOTA WARNING] "
				"Missing quota entry ID %d\n", dq->dq_id);
			scan_data.usage_is_inconsistent = 1;
		}
	}
	*usage_inconsistent = scan_data.usage_is_inconsistent;

out_close_qh:
	err = quota_file_close(qctx, &qh);
	if (err) {
		log_debug("Cannot close quotafile: %s", error_message(errno));
		if (qh.qh_qf.e2_file)
			ext2fs_file_close(qh.qh_qf.e2_file);
	}
out:
	return err;
}