static int calipso_doi_remove(u32 doi, struct netlbl_audit *audit_info)
{
	int ret_val;
	struct calipso_doi *doi_def;
	struct audit_buffer *audit_buf;

	spin_lock(&calipso_doi_list_lock);
	doi_def = calipso_doi_search(doi);
	if (!doi_def) {
		spin_unlock(&calipso_doi_list_lock);
		ret_val = -ENOENT;
		goto doi_remove_return;
	}
	if (!refcount_dec_and_test(&doi_def->refcount)) {
		spin_unlock(&calipso_doi_list_lock);
		ret_val = -EBUSY;
		goto doi_remove_return;
	}
	list_del_rcu(&doi_def->list);
	spin_unlock(&calipso_doi_list_lock);

	call_rcu(&doi_def->rcu, calipso_doi_free_rcu);
	ret_val = 0;

doi_remove_return:
	audit_buf = netlbl_audit_start(AUDIT_MAC_CALIPSO_DEL, audit_info);
	if (audit_buf) {
		audit_log_format(audit_buf,
				 " calipso_doi=%u res=%u",
				 doi, ret_val == 0 ? 1 : 0);
		audit_log_end(audit_buf);
	}

	return ret_val;
}