term_and_job_init(
	term_T	    *term,
	typval_T    *argvar,
	char	    **argv UNUSED,
	jobopt_T    *opt,
	jobopt_T    *orig_opt)
{
    WCHAR	    *cmd_wchar = NULL;
    WCHAR	    *cwd_wchar = NULL;
    WCHAR	    *env_wchar = NULL;
    channel_T	    *channel = NULL;
    job_T	    *job = NULL;
    DWORD	    error;
    HANDLE	    jo = NULL;
    HANDLE	    child_process_handle;
    HANDLE	    child_thread_handle;
    void	    *winpty_err = NULL;
    void	    *spawn_config = NULL;
    garray_T	    ga_cmd, ga_env;
    char_u	    *cmd = NULL;

    if (dyn_winpty_init(TRUE) == FAIL)
	return FAIL;
    ga_init2(&ga_cmd, (int)sizeof(char*), 20);
    ga_init2(&ga_env, (int)sizeof(char*), 20);

    if (argvar->v_type == VAR_STRING)
    {
	cmd = argvar->vval.v_string;
    }
    else if (argvar->v_type == VAR_LIST)
    {
	if (win32_build_cmd(argvar->vval.v_list, &ga_cmd) == FAIL)
	    goto failed;
	cmd = ga_cmd.ga_data;
    }
    if (cmd == NULL || *cmd == NUL)
    {
	EMSG(_(e_invarg));
	goto failed;
    }

    cmd_wchar = enc_to_utf16(cmd, NULL);
    ga_clear(&ga_cmd);
    if (cmd_wchar == NULL)
	goto failed;
    if (opt->jo_cwd != NULL)
	cwd_wchar = enc_to_utf16(opt->jo_cwd, NULL);

    win32_build_env(opt->jo_env, &ga_env, TRUE);
    env_wchar = ga_env.ga_data;

    term->tl_winpty_config = winpty_config_new(0, &winpty_err);
    if (term->tl_winpty_config == NULL)
	goto failed;

    winpty_config_set_mouse_mode(term->tl_winpty_config,
						    WINPTY_MOUSE_MODE_FORCE);
    winpty_config_set_initial_size(term->tl_winpty_config,
						 term->tl_cols, term->tl_rows);
    term->tl_winpty = winpty_open(term->tl_winpty_config, &winpty_err);
    if (term->tl_winpty == NULL)
	goto failed;

    spawn_config = winpty_spawn_config_new(
	    WINPTY_SPAWN_FLAG_AUTO_SHUTDOWN |
		WINPTY_SPAWN_FLAG_EXIT_AFTER_SHUTDOWN,
	    NULL,
	    cmd_wchar,
	    cwd_wchar,
	    env_wchar,
	    &winpty_err);
    if (spawn_config == NULL)
	goto failed;

    channel = add_channel();
    if (channel == NULL)
	goto failed;

    job = job_alloc();
    if (job == NULL)
	goto failed;
    if (argvar->v_type == VAR_STRING)
    {
	int argc;

	build_argv_from_string(cmd, &job->jv_argv, &argc);
    }
    else
    {
	int argc;

	build_argv_from_list(argvar->vval.v_list, &job->jv_argv, &argc);
    }

    if (opt->jo_set & JO_IN_BUF)
	job->jv_in_buf = buflist_findnr(opt->jo_io_buf[PART_IN]);

    if (!winpty_spawn(term->tl_winpty, spawn_config, &child_process_handle,
	    &child_thread_handle, &error, &winpty_err))
	goto failed;

    channel_set_pipes(channel,
	(sock_T)CreateFileW(
	    winpty_conin_name(term->tl_winpty),
	    GENERIC_WRITE, 0, NULL,
	    OPEN_EXISTING, 0, NULL),
	(sock_T)CreateFileW(
	    winpty_conout_name(term->tl_winpty),
	    GENERIC_READ, 0, NULL,
	    OPEN_EXISTING, 0, NULL),
	(sock_T)CreateFileW(
	    winpty_conerr_name(term->tl_winpty),
	    GENERIC_READ, 0, NULL,
	    OPEN_EXISTING, 0, NULL));

    /* Write lines with CR instead of NL. */
    channel->ch_write_text_mode = TRUE;

    jo = CreateJobObject(NULL, NULL);
    if (jo == NULL)
	goto failed;

    if (!AssignProcessToJobObject(jo, child_process_handle))
    {
	/* Failed, switch the way to terminate process with TerminateProcess. */
	CloseHandle(jo);
	jo = NULL;
    }

    winpty_spawn_config_free(spawn_config);
    vim_free(cmd_wchar);
    vim_free(cwd_wchar);
    vim_free(env_wchar);

    create_vterm(term, term->tl_rows, term->tl_cols);

#if defined(FEAT_GUI) || defined(FEAT_TERMGUICOLORS)
    if (opt->jo_set2 & JO2_ANSI_COLORS)
	set_vterm_palette(term->tl_vterm, opt->jo_ansi_colors);
    else
	init_vterm_ansi_colors(term->tl_vterm);
#endif

    channel_set_job(channel, job, opt);
    job_set_options(job, opt);

    job->jv_channel = channel;
    job->jv_proc_info.hProcess = child_process_handle;
    job->jv_proc_info.dwProcessId = GetProcessId(child_process_handle);
    job->jv_job_object = jo;
    job->jv_status = JOB_STARTED;
    job->jv_tty_in = utf16_to_enc(
	    (short_u*)winpty_conin_name(term->tl_winpty), NULL);
    job->jv_tty_out = utf16_to_enc(
	    (short_u*)winpty_conout_name(term->tl_winpty), NULL);
    ++job->jv_refcount;
    term->tl_job = job;

    /* Redirecting stdout and stderr doesn't work at the job level.  Instead
     * open the file here and handle it in.  opt->jo_io was changed in
     * setup_job_options(), use the original flags here. */
    if (orig_opt->jo_io[PART_OUT] == JIO_FILE)
    {
	char_u *fname = opt->jo_io_name[PART_OUT];

	ch_log(channel, "Opening output file %s", fname);
	term->tl_out_fd = mch_fopen((char *)fname, WRITEBIN);
	if (term->tl_out_fd == NULL)
	    EMSG2(_(e_notopen), fname);
    }

    return OK;

failed:
    ga_clear(&ga_cmd);
    ga_clear(&ga_env);
    vim_free(cmd_wchar);
    vim_free(cwd_wchar);
    if (spawn_config != NULL)
	winpty_spawn_config_free(spawn_config);
    if (channel != NULL)
	channel_clear(channel);
    if (job != NULL)
    {
	job->jv_channel = NULL;
	job_cleanup(job);
    }
    term->tl_job = NULL;
    if (jo != NULL)
	CloseHandle(jo);
    if (term->tl_winpty != NULL)
	winpty_free(term->tl_winpty);
    term->tl_winpty = NULL;
    if (term->tl_winpty_config != NULL)
	winpty_config_free(term->tl_winpty_config);
    term->tl_winpty_config = NULL;
    if (winpty_err != NULL)
    {
	char_u *msg = utf16_to_enc(
				(short_u *)winpty_error_msg(winpty_err), NULL);

	EMSG(msg);
	winpty_error_free(winpty_err);
    }
    return FAIL;
}