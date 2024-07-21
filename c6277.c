static void _slurm_rpc_allocate_pack(slurm_msg_t * msg)
{
	static int select_serial = -1;
	static int active_rpc_cnt = 0;
	int error_code = SLURM_SUCCESS, inx, pack_cnt = -1;
	DEF_TIMERS;
	job_desc_msg_t *job_desc_msg;
	List job_req_list = (List) msg->data;
	/* Locks: Read config, write job, write node, read partition */
	slurmctld_lock_t job_write_lock = {
		READ_LOCK, WRITE_LOCK, WRITE_LOCK, READ_LOCK, READ_LOCK };
	uid_t uid = g_slurm_auth_get_uid(msg->auth_cred,
					 slurmctld_config.auth_info);
	uint32_t job_uid = NO_VAL;
	struct job_record *job_ptr, *first_job_ptr = NULL;
	char *err_msg = NULL, **job_submit_user_msg = NULL;
	ListIterator iter;
	bool priv_user;
	List submit_job_list = NULL;
	uint32_t pack_job_id = 0, pack_job_offset = 0;
	hostset_t jobid_hostset = NULL;
	char tmp_str[32];
	List resp = NULL;
	slurm_addr_t resp_addr;
	char resp_host[16];
	uint16_t port;	/* dummy value */

	START_TIMER;

	if (select_serial == -1) {
		if (xstrcmp(slurmctld_conf.select_type, "select/serial"))
			select_serial = 0;
		else
			select_serial = 1;
	}
	if (slurmctld_config.submissions_disabled || (select_serial == 1)) {
		info("Submissions disabled on system");
		error_code = ESLURM_SUBMISSIONS_DISABLED;
		goto send_msg;
	}
	if (!_sched_backfill()) {
		error_code = ESLURM_NOT_SUPPORTED;
		goto send_msg;
	}
	if (!job_req_list || (list_count(job_req_list) == 0)) {
		info("REQUEST_JOB_PACK_ALLOCATION from uid=%d with empty job list",
		     uid);
		error_code = SLURM_ERROR;
		goto send_msg;
	}
	if (slurm_get_peer_addr(msg->conn_fd, &resp_addr) == 0) {
		slurm_get_ip_str(&resp_addr, &port,resp_host,sizeof(resp_host));
	} else {
		info("REQUEST_JOB_PACK_ALLOCATION from uid=%d , can't get peer addr",
		     uid);
		error_code = SLURM_ERROR;
		goto send_msg;
	}

	debug2("sched: Processing RPC: REQUEST_JOB_PACK_ALLOCATION from uid=%d",
	       uid);
	pack_cnt = list_count(job_req_list);
	job_submit_user_msg = xmalloc(sizeof(char *) * pack_cnt);
	priv_user = validate_slurm_user(uid);
	submit_job_list = list_create(NULL);
	_throttle_start(&active_rpc_cnt);
	lock_slurmctld(job_write_lock);
	inx = 0;
	iter = list_iterator_create(job_req_list);
	while ((job_desc_msg = (job_desc_msg_t *) list_next(iter))) {
		if (job_uid == NO_VAL)
			job_uid = job_desc_msg->user_id;
		if ((uid != job_desc_msg->user_id) && !priv_user) {
			error_code = ESLURM_USER_ID_MISSING;
			error("Security violation, REQUEST_JOB_PACK_ALLOCATION from uid=%d",
			      uid);
			break;
		}

		if ((job_desc_msg->alloc_node == NULL) ||
		    (job_desc_msg->alloc_node[0] == '\0')) {
			error_code = ESLURM_INVALID_NODE_NAME;
			error("REQUEST_JOB_PACK_ALLOCATION lacks alloc_node from uid=%d",
			      uid);
			break;
		}

		if (job_desc_msg->array_inx) {
			error_code = ESLURM_INVALID_ARRAY;
			break;
		}

		if (job_desc_msg->immediate) {
			error_code = ESLURM_CAN_NOT_START_IMMEDIATELY;
			break;
		}

		/* Locks are for job_submit plugin use */
		job_desc_msg->pack_job_offset = pack_job_offset;
		error_code = validate_job_create_req(job_desc_msg, uid,
						     &job_submit_user_msg[inx++]);
		if (error_code)
			break;

#if HAVE_ALPS_CRAY
		/*
		 * Catch attempts to nest salloc sessions. It is not possible to
		 * use an ALPS session which has the same alloc_sid, it fails
		 * even if PAGG container IDs are used.
		 */
		if (allocated_session_in_use(job_desc_msg)) {
			error_code = ESLURM_RESERVATION_BUSY;
			error("attempt to nest ALPS allocation on %s:%d by uid=%d",
			      job_desc_msg->alloc_node, job_desc_msg->alloc_sid,
			      uid);
			break;
		}
#endif
		dump_job_desc(job_desc_msg);

		job_ptr = NULL;
		if (!job_desc_msg->resp_host)
			job_desc_msg->resp_host = xstrdup(resp_host);
		if (pack_job_offset) {
			/* Email notifications disable except for pack leader */
			job_desc_msg->mail_type = 0;
			xfree(job_desc_msg->mail_user);
		}
		job_desc_msg->pack_job_offset = pack_job_offset;
		error_code = job_allocate(job_desc_msg, false, false, NULL,
					  true, uid, &job_ptr, &err_msg,
					  msg->protocol_version);
		if (!job_ptr) {
			if (error_code == SLURM_SUCCESS)
				error_code = SLURM_ERROR;
			break;
		}
		if (error_code && (job_ptr->job_state == JOB_FAILED))
			break;
		error_code = SLURM_SUCCESS;	/* Non-fatal error */
		if (pack_job_id == 0) {
			pack_job_id = job_ptr->job_id;
			first_job_ptr = job_ptr;
		}
		snprintf(tmp_str, sizeof(tmp_str), "%u", job_ptr->job_id);
		if (jobid_hostset)
			hostset_insert(jobid_hostset, tmp_str);
		else
			jobid_hostset = hostset_create(tmp_str);
		job_ptr->pack_job_id     = pack_job_id;
		job_ptr->pack_job_offset = pack_job_offset++;
		list_append(submit_job_list, job_ptr);
	}
	list_iterator_destroy(iter);

	if ((error_code == 0) && (!first_job_ptr)) {
		error("%s: No error, but no pack_job_id", __func__);
		error_code = SLURM_ERROR;
	}

	/* Validate limits on pack-job as a whole */
	if ((error_code == SLURM_SUCCESS) &&
	    (accounting_enforce & ACCOUNTING_ENFORCE_LIMITS) &&
	    !acct_policy_validate_pack(submit_job_list)) {
		info("Pack job %u exceeded association/QOS limit for user %u",
		     pack_job_id, job_uid);
		error_code = ESLURM_ACCOUNTING_POLICY;
        }

	if (error_code) {
		/* Cancel remaining job records */
		(void) list_for_each(submit_job_list, _pack_job_cancel, NULL);
		if (first_job_ptr)
			first_job_ptr->pack_job_list = submit_job_list;
		else
			FREE_NULL_LIST(submit_job_list);
	} else {
		resource_allocation_response_msg_t *alloc_msg;
		ListIterator iter;
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
		inx = 0;
		iter = list_iterator_create(submit_job_list);
		while ((job_ptr = (struct job_record *) list_next(iter))) {
			job_ptr->pack_job_id_set = xstrdup(tmp_offset);
			if (!resp)
				resp = list_create(_del_alloc_pack_msg);
			alloc_msg = xmalloc_nz(
				sizeof(resource_allocation_response_msg_t));
			_build_alloc_msg(job_ptr, alloc_msg, error_code,
					 job_submit_user_msg[inx++]);
			list_append(resp, alloc_msg);
			if (slurmctld_conf.debug_flags &
			    DEBUG_FLAG_HETERO_JOBS) {
				char buf[BUFSIZ];
				info("Submit %s",
				     jobid2fmt(job_ptr, buf, sizeof(buf)));
			}
		}
		list_iterator_destroy(iter);
		xfree(tmp_str);
	}
	unlock_slurmctld(job_write_lock);
	_throttle_fini(&active_rpc_cnt);
	END_TIMER2("_slurm_rpc_allocate_pack");

	if (resp) {
		slurm_msg_t response_msg;
		slurm_msg_t_init(&response_msg);
		response_msg.conn = msg->conn;
		response_msg.flags = msg->flags;
		response_msg.protocol_version = msg->protocol_version;
		response_msg.msg_type = RESPONSE_JOB_PACK_ALLOCATION;
		response_msg.data = resp;

		if (slurm_send_node_msg(msg->conn_fd, &response_msg) < 0)
			_kill_job_on_msg_fail(pack_job_id);
		list_destroy(resp);
	} else {
send_msg:	info("%s: %s ", __func__, slurm_strerror(error_code));
		if (err_msg)
			slurm_send_rc_err_msg(msg, error_code, err_msg);
		else
			slurm_send_rc_msg(msg, error_code);
	}
	xfree(err_msg);
	for (inx = 0; inx < pack_cnt; inx++)
		xfree(job_submit_user_msg[inx]);
	xfree(job_submit_user_msg);

	if (jobid_hostset)
		hostset_destroy(jobid_hostset);

	schedule_job_save();	/* has own locks */
}