_rpc_batch_job(slurm_msg_t *msg, bool new_msg)
{
	batch_job_launch_msg_t *req = (batch_job_launch_msg_t *)msg->data;
	bool     first_job_run;
	int      rc = SLURM_SUCCESS;
	bool	 replied = false, revoked;
	slurm_addr_t *cli = &msg->orig_addr;

	if (new_msg) {
		uid_t req_uid = g_slurm_auth_get_uid(msg->auth_cred,
						     conf->auth_info);
		if (!_slurm_authorized_user(req_uid)) {
			error("Security violation, batch launch RPC from uid %d",
			      req_uid);
			rc = ESLURM_USER_ID_MISSING;  /* or bad in this case */
			goto done;
		}
	}

	if (_launch_job_test(req->job_id)) {
		error("Job %u already running, do not launch second copy",
		      req->job_id);
		rc = ESLURM_DUPLICATE_JOB_ID;	/* job already running */
		_launch_job_fail(req->job_id, rc);
		goto done;
	}

	slurm_cred_handle_reissue(conf->vctx, req->cred);
	if (slurm_cred_revoked(conf->vctx, req->cred)) {
		error("Job %u already killed, do not launch batch job",
		      req->job_id);
		rc = ESLURMD_CREDENTIAL_REVOKED;	/* job already ran */
		goto done;
	}

	task_g_slurmd_batch_request(req->job_id, req);	/* determine task affinity */

	slurm_mutex_lock(&prolog_mutex);
	first_job_run = !slurm_cred_jobid_cached(conf->vctx, req->job_id);

	/* BlueGene prolog waits for partition boot and is very slow.
	 * On any system we might need to load environment variables
	 * for Moab (see --get-user-env), which could also be slow.
	 * Just reply now and send a separate kill job request if the
	 * prolog or launch fail. */
	replied = true;
	if (new_msg && (slurm_send_rc_msg(msg, rc) < 1)) {
		/* The slurmctld is no longer waiting for a reply.
		 * This typically indicates that the slurmd was
		 * blocked from memory and/or CPUs and the slurmctld
		 * has requeued the batch job request. */
		error("Could not confirm batch launch for job %u, "
		      "aborting request", req->job_id);
		rc = SLURM_COMMUNICATIONS_SEND_ERROR;
		slurm_mutex_unlock(&prolog_mutex);
		goto done;
	}

	/*
	 * Insert jobid into credential context to denote that
	 * we've now "seen" an instance of the job
	 */
	if (first_job_run) {
		job_env_t job_env;
		slurm_cred_insert_jobid(conf->vctx, req->job_id);
		_add_job_running_prolog(req->job_id);
		slurm_mutex_unlock(&prolog_mutex);

		memset(&job_env, 0, sizeof(job_env_t));

		job_env.jobid = req->job_id;
		job_env.step_id = req->step_id;
		job_env.node_list = req->nodes;
		job_env.partition = req->partition;
		job_env.spank_job_env = req->spank_job_env;
		job_env.spank_job_env_size = req->spank_job_env_size;
		job_env.uid = req->uid;
		job_env.user_name = req->user_name;
		/*
	 	 * Run job prolog on this node
	 	 */
#if defined(HAVE_BG)
		select_g_select_jobinfo_get(req->select_jobinfo,
					    SELECT_JOBDATA_BLOCK_ID,
					    &job_env.resv_id);
#elif defined(HAVE_ALPS_CRAY)
		job_env.resv_id = select_g_select_jobinfo_xstrdup(
			req->select_jobinfo, SELECT_PRINT_RESV_ID);
#endif
		if (container_g_create(req->job_id))
			error("container_g_create(%u): %m", req->job_id);
		rc = _run_prolog(&job_env, req->cred);
		xfree(job_env.resv_id);
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
			_prolog_error(req, rc);
			rc = ESLURMD_PROLOG_FAILED;
			goto done;
		}
	} else {
		slurm_mutex_unlock(&prolog_mutex);
		_wait_for_job_running_prolog(req->job_id);
	}

	if (_get_user_env(req) < 0) {
		bool requeue = _requeue_setup_env_fail();
		if (requeue) {
			rc = ESLURMD_SETUP_ENVIRONMENT_ERROR;
			goto done;
		}
	}
	_set_batch_job_limits(msg);

	/* Since job could have been killed while the prolog was
	 * running (especially on BlueGene, which can take minutes
	 * for partition booting). Test if the credential has since
	 * been revoked and exit as needed. */
	if (slurm_cred_revoked(conf->vctx, req->cred)) {
		info("Job %u already killed, do not launch batch job",
		     req->job_id);
		rc = ESLURMD_CREDENTIAL_REVOKED;     /* job already ran */
		goto done;
	}

	slurm_mutex_lock(&launch_mutex);
	if (req->step_id == SLURM_BATCH_SCRIPT)
		info("Launching batch job %u for UID %d",
		     req->job_id, req->uid);
	else
		info("Launching batch job %u.%u for UID %d",
		     req->job_id, req->step_id, req->uid);

	debug3("_rpc_batch_job: call to _forkexec_slurmstepd");
	rc = _forkexec_slurmstepd(LAUNCH_BATCH_JOB, (void *)req, cli, NULL,
				  (hostset_t)NULL, SLURM_PROTOCOL_VERSION);
	debug3("_rpc_batch_job: return from _forkexec_slurmstepd: %d", rc);

	slurm_mutex_unlock(&launch_mutex);
	_launch_complete_add(req->job_id);

	/* On a busy system, slurmstepd may take a while to respond,
	 * if the job was cancelled in the interim, run through the
	 * abort logic below. */
	revoked = slurm_cred_revoked(conf->vctx, req->cred);
	if (revoked)
		_launch_complete_rm(req->job_id);
	if (revoked && _is_batch_job_finished(req->job_id)) {
		/* If configured with select/serial and the batch job already
		 * completed, consider the job sucessfully launched and do
		 * not repeat termination logic below, which in the worst case
		 * just slows things down with another message. */
		revoked = false;
	}
	if (revoked) {
		info("Job %u killed while launch was in progress",
		     req->job_id);
		sleep(1);	/* give slurmstepd time to create
				 * the communication socket */
		_terminate_all_steps(req->job_id, true);
		rc = ESLURMD_CREDENTIAL_REVOKED;
		goto done;
	}

done:
	if (!replied) {
		if (new_msg && (slurm_send_rc_msg(msg, rc) < 1)) {
			/* The slurmctld is no longer waiting for a reply.
			 * This typically indicates that the slurmd was
			 * blocked from memory and/or CPUs and the slurmctld
			 * has requeued the batch job request. */
			error("Could not confirm batch launch for job %u, "
			      "aborting request", req->job_id);
			rc = SLURM_COMMUNICATIONS_SEND_ERROR;
		} else {
			/* No need to initiate separate reply below */
			rc = SLURM_SUCCESS;
		}
	}
	if (rc != SLURM_SUCCESS) {
		/* prolog or job launch failure,
		 * tell slurmctld that the job failed */
		if (req->step_id == SLURM_BATCH_SCRIPT)
			_launch_job_fail(req->job_id, rc);
		else
			_abort_step(req->job_id, req->step_id);
	}

	/*
	 *  If job prolog failed or we could not reply,
	 *  initiate message to slurmctld with current state
	 */
	if ((rc == ESLURMD_PROLOG_FAILED)
	    || (rc == SLURM_COMMUNICATIONS_SEND_ERROR)
	    || (rc == ESLURMD_SETUP_ENVIRONMENT_ERROR)) {
		send_registration_msg(rc, false);
	}
}