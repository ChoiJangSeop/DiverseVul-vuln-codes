static bool _is_valid_will_run_user(job_desc_msg_t *job_desc_msg, uid_t uid)
{
	char *account = NULL;

	if ((uid == job_desc_msg->user_id) || validate_operator(uid))
		return true;

	if (job_desc_msg->job_id != NO_VAL) {
		struct job_record *job_ptr;
		job_ptr = find_job_record(job_desc_msg->job_id);
		if (job_ptr)
			account = job_ptr->account;
	} else if (job_desc_msg->account)
		account = job_desc_msg->account;

	if (account && assoc_mgr_is_user_acct_coord(acct_db_conn, uid, account))
		return true;

	return false;
}