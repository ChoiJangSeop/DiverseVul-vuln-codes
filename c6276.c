static void _slurm_rpc_submit_batch_pack_job(slurm_msg_t *msg)
{
	static int select_serial = -1;
	static int active_rpc_cnt = 0;
	ListIterator iter;
	int error_code = SLURM_SUCCESS, alloc_only = 0;
	DEF_TIMERS;
	uint32_t pack_job_id = 0, pack_job_offset = 0;
	struct job_record *job_ptr = NULL, *first_job_ptr = NULL;
	slurm_msg_t response_msg;
	submit_response_msg_t submit_msg;
	job_desc_msg_t *job_desc_msg;
	char *script = NULL;
	/* Locks: Read config, read job, read node, read partition */
	slurmctld_lock_t job_read_lock = {
		READ_LOCK, READ_LOCK, READ_LOCK, READ_LOCK, NO_LOCK };
	/* Locks: Read config, write job, write node, read partition */
	slurmctld_lock_t job_write_lock = {
		READ_LOCK, WRITE_LOCK, WRITE_LOCK, READ_LOCK, NO_LOCK };
	List job_req_list = (List) msg->data;
	uid_t uid = g_slurm_auth_get_uid(msg->auth_cred,
					 slurmctld_config.auth_info);
	uint32_t job_uid = NO_VAL;
	char *err_msg = NULL, *job_submit_user_msg = NULL;
	bool reject_job = false;
	bool is_super_user;
	List submit_job_list = NULL;
	hostset_t jobid_hostset = NULL;
	char tmp_str[32];

	START_TIMER;
	debug2("Processing RPC: REQUEST_SUBMIT_BATCH_PACK_JOB from uid=%d",
	       uid);
	if (!job_req_list || (list_count(job_req_list) == 0)) {
		info("REQUEST_SUBMIT_BATCH_PACK_JOB from uid=%d with empty job list",
		     uid);
		error_code = SLURM_ERROR;
		reject_job = true;
		goto send_msg;
	}
	if (!_sched_backfill()) {
		error_code = ESLURM_NOT_SUPPORTED;
		reject_job = true;
		goto send_msg;
	}
	if (select_serial == -1) {
		if (xstrcmp(slurmctld_conf.select_type, "select/serial"))
			select_serial = 0;
		else
			select_serial = 1;
	}
	if (slurmctld_config.submissions_disabled || (select_serial == 1)) {
		info("Submissions disabled on system");
		error_code = ESLURM_SUBMISSIONS_DISABLED;
		reject_job = true;
		goto send_msg;
	}
	if (!job_req_list || (list_count(job_req_list) == 0)) {
		info("REQUEST_SUBMIT_BATCH_PACK_JOB from uid=%d with empty job list",
		     uid);
		error_code = SLURM_ERROR;
		reject_job = true;
		goto send_msg;
	}

	/* Validate the individual request */
	is_super_user = validate_super_user(uid);
	lock_slurmctld(job_read_lock);     /* Locks for job_submit plugin use */
	iter = list_iterator_create(job_req_list);
	while ((job_desc_msg = (job_desc_msg_t *) list_next(iter))) {
		if (job_uid == NO_VAL)
			job_uid = job_desc_msg->user_id;
		if ((uid != job_desc_msg->user_id) && !is_super_user) {
			/* NOTE: Super root can submit a job for any user */
			error("Security violation, REQUEST_SUBMIT_BATCH_PACK_JOB from uid=%d",
			      uid);
			error_code = ESLURM_USER_ID_MISSING;
			break;
		}
		if ((job_desc_msg->alloc_node == NULL) ||
		    (job_desc_msg->alloc_node[0] == '\0')) {
			error("REQUEST_SUBMIT_BATCH_PACK_JOB lacks alloc_node from uid=%d",
			      uid);
			error_code = ESLURM_INVALID_NODE_NAME;
			break;
		}
		dump_job_desc(job_desc_msg);

		job_desc_msg->pack_job_offset = pack_job_offset;
		error_code = validate_job_create_req(job_desc_msg, uid,
						     &err_msg);
		if (error_code != SLURM_SUCCESS) {
			reject_job = true;
			break;
		}

		if (err_msg) {
			char *save_ptr = NULL, *tok;
			tok = strtok_r(err_msg, "\n", &save_ptr);
			while (tok) {
				char *sep = "";
				if (job_submit_user_msg)
					sep = "\n";
				xstrfmtcat(job_submit_user_msg, "%s%d: %s",
					   sep, pack_job_offset, tok);
				tok = strtok_r(NULL, "\n", &save_ptr);
			}
			xfree(err_msg);
		}
		pack_job_offset++;
	}
	list_iterator_destroy(iter);
	unlock_slurmctld(job_read_lock);
	if (error_code != SLURM_SUCCESS)
		goto send_msg;

	/*
	 * In validate_job_create_req, err_msg is currently only modified in
	 * the call to job_submit_plugin_submit. We save the err_msg in a temp
	 * char *job_submit_user_msg because err_msg can be overwritten later
	 * in the calls to job_allocate, and we need the job submit plugin value
	 * to build the resource allocation response in the call to
	 * _build_alloc_msg.
	 */
	if (err_msg)
		job_submit_user_msg = xstrdup(err_msg);

	/* Create new job allocations */
	submit_job_list = list_create(NULL);
	pack_job_offset = 0;
	_throttle_start(&active_rpc_cnt);
	lock_slurmctld(job_write_lock);
	START_TIMER;	/* Restart after we have locks */
	iter = list_iterator_create(job_req_list);
	while ((job_desc_msg = (job_desc_msg_t *) list_next(iter))) {
		if (!script)
			script = xstrdup(job_desc_msg->script);
		if (pack_job_offset && job_desc_msg->script) {
			info("%s: Pack job %u offset %u has script, being ignored",
			     __func__, pack_job_id, pack_job_offset);
			xfree(job_desc_msg->script);

		}
		if (pack_job_offset) {
			/* Email notifications disable except for pack leader */
			job_desc_msg->mail_type = 0;
			xfree(job_desc_msg->mail_user);
		}
		if (!job_desc_msg->burst_buffer) {
			xfree(job_desc_msg->script);
			job_desc_msg->script =
				bb_g_build_pack_script(script, pack_job_offset);
		}
		job_desc_msg->pack_job_offset = pack_job_offset;
		error_code = job_allocate(job_desc_msg,
					  job_desc_msg->immediate, false,
					  NULL, alloc_only, uid, &job_ptr,
					  &err_msg, msg->protocol_version);
		if (!job_ptr ||
		    (error_code && job_ptr->job_state == JOB_FAILED)) {
			reject_job = true;
		} else {
			if (pack_job_id == 0) {
				pack_job_id = job_ptr->job_id;
				first_job_ptr = job_ptr;
				alloc_only = 1;
			}
			snprintf(tmp_str, sizeof(tmp_str), "%u",
				 job_ptr->job_id);
			if (jobid_hostset)
				hostset_insert(jobid_hostset, tmp_str);
			else
				jobid_hostset = hostset_create(tmp_str);
			job_ptr->pack_job_id     = pack_job_id;
			job_ptr->pack_job_offset = pack_job_offset++;
			job_ptr->batch_flag      = 1;
			list_append(submit_job_list, job_ptr);
		}

		if (job_desc_msg->immediate &&
		    (error_code != SLURM_SUCCESS)) {
			error_code = ESLURM_CAN_NOT_START_IMMEDIATELY;
			reject_job = true;
		}
		if (reject_job)
			break;
	}
	list_iterator_destroy(iter);
	xfree(script);

	if ((pack_job_id == 0) && !reject_job) {
		info("%s: No error, but no pack_job_id", __func__);
		error_code = SLURM_ERROR;
		reject_job = true;
	}

	/* Validate limits on pack-job as a whole */
	if (!reject_job &&
	    (accounting_enforce & ACCOUNTING_ENFORCE_LIMITS) &&
	    !acct_policy_validate_pack(submit_job_list)) {
		info("Pack job %u exceeded association/QOS limit for user %u",
		     pack_job_id, job_uid);
		error_code = ESLURM_ACCOUNTING_POLICY;
		reject_job = true;
	}

	if (!reject_job) {
		int buf_size = pack_job_offset * 16;
		char *tmp_str = xmalloc(buf_size);
		char *tmp_offset = tmp_str;
		first_job_ptr->pack_job_list = submit_job_list;
		hostset_ranged_string(jobid_hostset, buf_size, tmp_str);
		if (tmp_str[0] == '[') {
			tmp_offset = strchr(tmp_str, ']');
			if (tmp_offset)
				tmp_offset[0] = '\0';
			tmp_offset = tmp_str + 1;
		}
		iter = list_iterator_create(submit_job_list);
		while ((job_ptr = (struct job_record *) list_next(iter))) {
			job_ptr->pack_job_id_set = xstrdup(tmp_offset);
			if (slurmctld_conf.debug_flags &
			    DEBUG_FLAG_HETERO_JOBS) {
				char buf[JBUFSIZ];
				info("Submit %s",
				     jobid2fmt(job_ptr, buf, sizeof(buf)));
			}
		}
		list_iterator_destroy(iter);
		xfree(tmp_str);
	}

	unlock_slurmctld(job_write_lock);
	_throttle_fini(&active_rpc_cnt);

send_msg:
	END_TIMER2("_slurm_rpc_submit_batch_pack_job");
	if (reject_job) {
		info("%s: %s", __func__, slurm_strerror(error_code));
		if (err_msg)
			slurm_send_rc_err_msg(msg, error_code, err_msg);
		else
			slurm_send_rc_msg(msg, error_code);
		if (submit_job_list) {
			(void) list_for_each(submit_job_list, _pack_job_cancel,
					     NULL);
			if (first_job_ptr)
				first_job_ptr->pack_job_list = submit_job_list;
			else
				FREE_NULL_LIST(submit_job_list);
		}
	} else {
		info("%s: JobId=%u %s", __func__, pack_job_id, TIME_STR);
		/* send job_ID */
		submit_msg.job_id     = pack_job_id;
		submit_msg.step_id    = SLURM_BATCH_SCRIPT;
		submit_msg.error_code = error_code;
		submit_msg.job_submit_user_msg = job_submit_user_msg;
		slurm_msg_t_init(&response_msg);
		response_msg.flags = msg->flags;
		response_msg.protocol_version = msg->protocol_version;
		response_msg.conn = msg->conn;
		response_msg.msg_type = RESPONSE_SUBMIT_BATCH_JOB;
		response_msg.data = &submit_msg;
		slurm_send_node_msg(msg->conn_fd, &response_msg);

		schedule_job_save();	/* Has own locks */
	}
	if (jobid_hostset)
		hostset_destroy(jobid_hostset);
	xfree(err_msg);
	xfree(job_submit_user_msg);
}