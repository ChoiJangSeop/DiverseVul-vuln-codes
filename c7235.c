term_and_job_init(
	term_T	    *term,
	typval_T    *argvar,
	char	    **argv,
	jobopt_T    *opt,
	jobopt_T    *orig_opt UNUSED)
{
    create_vterm(term, term->tl_rows, term->tl_cols);

#if defined(FEAT_GUI) || defined(FEAT_TERMGUICOLORS)
    if (opt->jo_set2 & JO2_ANSI_COLORS)
	set_vterm_palette(term->tl_vterm, opt->jo_ansi_colors);
    else
	init_vterm_ansi_colors(term->tl_vterm);
#endif

    /* This may change a string in "argvar". */
    term->tl_job = job_start(argvar, argv, opt, TRUE);
    if (term->tl_job != NULL)
	++term->tl_job->jv_refcount;

    return term->tl_job != NULL
	&& term->tl_job->jv_channel != NULL
	&& term->tl_job->jv_status != JOB_FAILED ? OK : FAIL;
}