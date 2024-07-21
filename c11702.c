clear_evalarg(evalarg_T *evalarg, exarg_T *eap)
{
    if (evalarg != NULL)
    {
	if (evalarg->eval_tofree != NULL)
	{
	    if (eap != NULL)
	    {
		// We may need to keep the original command line, e.g. for
		// ":let" it has the variable names.  But we may also need the
		// new one, "nextcmd" points into it.  Keep both.
		vim_free(eap->cmdline_tofree);
		eap->cmdline_tofree = *eap->cmdlinep;
		*eap->cmdlinep = evalarg->eval_tofree;
	    }
	    else
		vim_free(evalarg->eval_tofree);
	    evalarg->eval_tofree = NULL;
	}

	ga_clear_strings(&evalarg->eval_tofree_ga);
	VIM_CLEAR(evalarg->eval_tofree_lambda);
    }
}