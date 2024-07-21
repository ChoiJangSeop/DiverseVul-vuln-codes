create_pty_only(term_T *term, jobopt_T *options)
{
    HANDLE	    hPipeIn = INVALID_HANDLE_VALUE;
    HANDLE	    hPipeOut = INVALID_HANDLE_VALUE;
    char	    in_name[80], out_name[80];
    channel_T	    *channel = NULL;

    create_vterm(term, term->tl_rows, term->tl_cols);

    vim_snprintf(in_name, sizeof(in_name), "\\\\.\\pipe\\vim-%d-in-%d",
	    GetCurrentProcessId(),
	    curbuf->b_fnum);
    hPipeIn = CreateNamedPipe(in_name, PIPE_ACCESS_OUTBOUND,
	    PIPE_TYPE_MESSAGE | PIPE_NOWAIT,
	    PIPE_UNLIMITED_INSTANCES,
	    0, 0, NMPWAIT_NOWAIT, NULL);
    if (hPipeIn == INVALID_HANDLE_VALUE)
	goto failed;

    vim_snprintf(out_name, sizeof(out_name), "\\\\.\\pipe\\vim-%d-out-%d",
	    GetCurrentProcessId(),
	    curbuf->b_fnum);
    hPipeOut = CreateNamedPipe(out_name, PIPE_ACCESS_INBOUND,
	    PIPE_TYPE_MESSAGE | PIPE_NOWAIT,
	    PIPE_UNLIMITED_INSTANCES,
	    0, 0, 0, NULL);
    if (hPipeOut == INVALID_HANDLE_VALUE)
	goto failed;

    ConnectNamedPipe(hPipeIn, NULL);
    ConnectNamedPipe(hPipeOut, NULL);

    term->tl_job = job_alloc();
    if (term->tl_job == NULL)
	goto failed;
    ++term->tl_job->jv_refcount;

    /* behave like the job is already finished */
    term->tl_job->jv_status = JOB_FINISHED;

    channel = add_channel();
    if (channel == NULL)
	goto failed;
    term->tl_job->jv_channel = channel;
    channel->ch_keep_open = TRUE;
    channel->ch_named_pipe = TRUE;

    channel_set_pipes(channel,
	(sock_T)hPipeIn,
	(sock_T)hPipeOut,
	(sock_T)hPipeOut);
    channel_set_job(channel, term->tl_job, options);
    term->tl_job->jv_tty_in = vim_strsave((char_u*)in_name);
    term->tl_job->jv_tty_out = vim_strsave((char_u*)out_name);

    return OK;

failed:
    if (hPipeIn != NULL)
	CloseHandle(hPipeIn);
    if (hPipeOut != NULL)
	CloseHandle(hPipeOut);
    return FAIL;
}