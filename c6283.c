_build_env(job_env_t *job_env)
{
	char **env = xmalloc(sizeof(char *));
	bool user_name_set = 0;

	env[0]  = NULL;
	if (!valid_spank_job_env(job_env->spank_job_env,
				 job_env->spank_job_env_size,
				 job_env->uid)) {
		/* If SPANK job environment is bad, log it and do not use */
		job_env->spank_job_env_size = 0;
		job_env->spank_job_env = (char **) NULL;
	}
	if (job_env->spank_job_env_size)
		env_array_merge(&env, (const char **) job_env->spank_job_env);

	slurm_mutex_lock(&conf->config_mutex);
	setenvf(&env, "SLURMD_NODENAME", "%s", conf->node_name);
	setenvf(&env, "SLURM_CONF", conf->conffile);
	slurm_mutex_unlock(&conf->config_mutex);

	setenvf(&env, "SLURM_CLUSTER_NAME", "%s", conf->cluster_name);
	setenvf(&env, "SLURM_JOB_ID", "%u", job_env->jobid);
	setenvf(&env, "SLURM_JOB_UID",   "%u", job_env->uid);

#ifndef HAVE_NATIVE_CRAY
	/* uid_to_string on a cray is a heavy call, so try to avoid it */
	if (!job_env->user_name) {
		job_env->user_name = uid_to_string(job_env->uid);
		user_name_set = 1;
	}
#endif

	setenvf(&env, "SLURM_JOB_USER", "%s", job_env->user_name);
	if (user_name_set)
		xfree(job_env->user_name);

	setenvf(&env, "SLURM_JOBID", "%u", job_env->jobid);
	setenvf(&env, "SLURM_UID",   "%u", job_env->uid);
	if (job_env->node_list)
		setenvf(&env, "SLURM_NODELIST", "%s", job_env->node_list);

	if (job_env->partition)
		setenvf(&env, "SLURM_JOB_PARTITION", "%s", job_env->partition);

	if (job_env->resv_id) {
#if defined(HAVE_BG)
		setenvf(&env, "MPIRUN_PARTITION", "%s", job_env->resv_id);
#elif defined(HAVE_ALPS_CRAY)
		setenvf(&env, "BASIL_RESERVATION_ID", "%s", job_env->resv_id);
#endif
	}
	return env;
}