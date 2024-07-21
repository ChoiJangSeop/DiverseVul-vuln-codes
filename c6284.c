_rpc_launch_tasks(slurm_msg_t *msg)
{
	int      errnum = SLURM_SUCCESS;
	uint16_t port;
	char     host[MAXHOSTNAMELEN];
	uid_t    req_uid;
	launch_tasks_request_msg_t *req = msg->data;
	bool     super_user = false;
	bool     mem_sort = false;
#ifndef HAVE_FRONT_END
	bool     first_job_run;
#endif
	slurm_addr_t self;
	slurm_addr_t *cli = &msg->orig_addr;
	hostset_t step_hset = NULL;
	job_mem_limits_t *job_limits_ptr;
	int nodeid = 0;
	bitstr_t *numa_bitmap = NULL;

#ifndef HAVE_FRONT_END
	/* It is always 0 for front end systems */
	nodeid = nodelist_find(req->complete_nodelist, conf->node_name);
#endif
	req_uid = g_slurm_auth_get_uid(msg->auth_cred, conf->auth_info);
	memcpy(&req->orig_addr, &msg->orig_addr, sizeof(slurm_addr_t));

	super_user = _slurm_authorized_user(req_uid);

	if ((super_user == false) && (req_uid != req->uid)) {
		error("launch task request from uid %u",
		      (unsigned int) req_uid);
		errnum = ESLURM_USER_ID_MISSING;	/* or invalid user */
		goto done;
	}

	slurm_get_ip_str(cli, &port, host, sizeof(host));
	info("launch task %u.%u request from %u.%u@%s (port %hu)", req->job_id,
	     req->job_step_id, req->uid, req->gid, host, port);

	/* this could be set previously and needs to be overwritten by
	 * this call for messages to work correctly for the new call */
	env_array_overwrite(&req->env, "SLURM_SRUN_COMM_HOST", host);
	req->envc = envcount(req->env);

#ifndef HAVE_FRONT_END
	slurm_mutex_lock(&prolog_mutex);
	first_job_run = !slurm_cred_jobid_cached(conf->vctx, req->job_id);
#endif
	if (_check_job_credential(req, req_uid, nodeid, &step_hset,
				  msg->protocol_version) < 0) {
		errnum = errno;
		error("Invalid job credential from %ld@%s: %m",
		      (long) req_uid, host);
#ifndef HAVE_FRONT_END
		slurm_mutex_unlock(&prolog_mutex);
#endif
		goto done;
	}

	/* Must follow _check_job_credential(), which sets some req fields */
	task_g_slurmd_launch_request(req->job_id, req, nodeid);

#ifndef HAVE_FRONT_END
	if (first_job_run) {
		int rc;
		job_env_t job_env;

		slurm_cred_insert_jobid(conf->vctx, req->job_id);
		_add_job_running_prolog(req->job_id);
		slurm_mutex_unlock(&prolog_mutex);

		if (container_g_create(req->job_id))
			error("container_g_create(%u): %m", req->job_id);

		memset(&job_env, 0, sizeof(job_env_t));

		job_env.jobid = req->job_id;
		job_env.step_id = req->job_step_id;
		job_env.node_list = req->complete_nodelist;
		job_env.partition = req->partition;
		job_env.spank_job_env = req->spank_job_env;
		job_env.spank_job_env_size = req->spank_job_env_size;
		job_env.uid = req->uid;
		job_env.user_name = req->user_name;
		rc =  _run_prolog(&job_env, req->cred);
		if (rc) {
			int term_sig, exit_status;
			if (WIFSIGNALED(rc)) {
				exit_status = 0;
				term_sig    = WTERMSIG(rc);
			} else {
				exit_status = WEXITSTATUS(rc);
				term_sig    = 0;
			}
			error("[job %u] prolog failed status=%d:%d",
			      req->job_id, exit_status, term_sig);
			errnum = ESLURMD_PROLOG_FAILED;
			goto done;
		}
		/* Since the job could have been killed while the prolog was
		 * running, test if the credential has since been revoked
		 * and exit as needed. */
		if (slurm_cred_revoked(conf->vctx, req->cred)) {
			info("Job %u already killed, do not launch step %u.%u",
			     req->job_id, req->job_id, req->job_step_id);
			errnum = ESLURMD_CREDENTIAL_REVOKED;
			goto done;
		}
	} else {
		slurm_mutex_unlock(&prolog_mutex);
		_wait_for_job_running_prolog(req->job_id);
	}
#endif

	if (req->mem_bind_type & MEM_BIND_SORT) {
		int task_cnt = -1;
		if (req->tasks_to_launch)
			task_cnt = (int) req->tasks_to_launch[nodeid];
		mem_sort = true;
		numa_bitmap = _build_numa_bitmap(req->mem_bind_type,
						 req->mem_bind,
						 req->cpu_bind_type,
						 req->cpu_bind, task_cnt);
	}
	node_features_g_step_config(mem_sort, numa_bitmap);
	FREE_NULL_BITMAP(numa_bitmap);

	if (req->job_mem_lim || req->step_mem_lim) {
		step_loc_t step_info;
		slurm_mutex_lock(&job_limits_mutex);
		if (!job_limits_list)
			job_limits_list = list_create(_job_limits_free);
		step_info.jobid  = req->job_id;
		step_info.stepid = req->job_step_id;
		job_limits_ptr = list_find_first (job_limits_list,
						  _step_limits_match,
						  &step_info);
		if (!job_limits_ptr) {
			job_limits_ptr = xmalloc(sizeof(job_mem_limits_t));
			job_limits_ptr->job_id   = req->job_id;
			job_limits_ptr->job_mem  = req->job_mem_lim;
			job_limits_ptr->step_id  = req->job_step_id;
			job_limits_ptr->step_mem = req->step_mem_lim;
#if _LIMIT_INFO
			info("AddLim step:%u.%u job_mem:%"PRIu64" "
			     "step_mem:%"PRIu64"",
			     job_limits_ptr->job_id, job_limits_ptr->step_id,
			     job_limits_ptr->job_mem,
			     job_limits_ptr->step_mem);
#endif
			list_append(job_limits_list, job_limits_ptr);
		}
		slurm_mutex_unlock(&job_limits_mutex);
	}

	slurm_get_stream_addr(msg->conn_fd, &self);

	debug3("_rpc_launch_tasks: call to _forkexec_slurmstepd");
	errnum = _forkexec_slurmstepd(LAUNCH_TASKS, (void *)req, cli, &self,
				      step_hset, msg->protocol_version);
	debug3("_rpc_launch_tasks: return from _forkexec_slurmstepd");
	_launch_complete_add(req->job_id);

    done:
	if (step_hset)
		hostset_destroy(step_hset);

	if (slurm_send_rc_msg(msg, errnum) < 0) {
		char addr_str[32];
		slurm_print_slurm_addr(&msg->address, addr_str,
				       sizeof(addr_str));
		error("_rpc_launch_tasks: unable to send return code to "
		      "address:port=%s msg_type=%u: %m",
		      addr_str, msg->msg_type);

		/*
		 * Rewind credential so that srun may perform retry
		 */
		slurm_cred_rewind(conf->vctx, req->cred); /* ignore errors */

	} else if (errnum == SLURM_SUCCESS) {
		save_cred_state(conf->vctx);
		task_g_slurmd_reserve_resources(req->job_id, req, nodeid);
	}

	/*
	 *  If job prolog failed, indicate failure to slurmctld
	 */
	if (errnum == ESLURMD_PROLOG_FAILED)
		send_registration_msg(errnum, false);
}