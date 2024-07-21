create_pty_only(term_T *term, jobopt_T *opt)
{
    create_vterm(term, term->tl_rows, term->tl_cols);

    term->tl_job = job_alloc();
    if (term->tl_job == NULL)
	return FAIL;
    ++term->tl_job->jv_refcount;

    /* behave like the job is already finished */
    term->tl_job->jv_status = JOB_FINISHED;

    return mch_create_pty_channel(term->tl_job, opt);
}