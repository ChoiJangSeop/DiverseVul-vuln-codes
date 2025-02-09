ex_finally(exarg_T *eap)
{
    int		idx;
    int		skip = FALSE;
    int		pending = CSTP_NONE;
    cstack_T	*cstack = eap->cstack;

    if (cmdmod_error(FALSE))
	return;

    if (cstack->cs_trylevel <= 0 || cstack->cs_idx < 0)
	eap->errmsg = _(e_finally_without_try);
    else
    {
	if (!(cstack->cs_flags[cstack->cs_idx] & CSF_TRY))
	{
	    eap->errmsg = get_end_emsg(cstack);
	    for (idx = cstack->cs_idx - 1; idx > 0; --idx)
		if (cstack->cs_flags[idx] & CSF_TRY)
		    break;
	    // Make this error pending, so that the commands in the following
	    // finally clause can be executed.  This overrules also a pending
	    // ":continue", ":break", ":return", or ":finish".
	    pending = CSTP_ERROR;
	}
	else
	    idx = cstack->cs_idx;

	if (cstack->cs_flags[idx] & CSF_FINALLY)
	{
	    // Give up for a multiple ":finally" and ignore it.
	    eap->errmsg = _(e_multiple_finally);
	    return;
	}
	rewind_conditionals(cstack, idx, CSF_WHILE | CSF_FOR,
						       &cstack->cs_looplevel);

	/*
	 * Don't do something when the corresponding try block never got active
	 * (because of an inactive surrounding conditional or after an error or
	 * interrupt or throw) or for a ":finally" without ":try" or a multiple
	 * ":finally".  After every other error (did_emsg or the conditional
	 * errors detected above) or after an interrupt (got_int) or an
	 * exception (did_throw), the finally clause must be executed.
	 */
	skip = !(cstack->cs_flags[cstack->cs_idx] & CSF_TRUE);

	if (!skip)
	{
	    // When debugging or a breakpoint was encountered, display the
	    // debug prompt (if not already done).  The user then knows that the
	    // finally clause is executed.
	    if (dbg_check_skipped(eap))
	    {
		// Handle a ">quit" debug command as if an interrupt had
		// occurred before the ":finally".  That is, discard the
		// original exception and replace it by an interrupt
		// exception.
		(void)do_intthrow(cstack);
	    }

	    /*
	     * If there is a preceding catch clause and it caught the exception,
	     * finish the exception now.  This happens also after errors except
	     * when this is a multiple ":finally" or one not within a ":try".
	     * After an error or interrupt, this also discards a pending
	     * ":continue", ":break", ":finish", or ":return" from the preceding
	     * try block or catch clause.
	     */
	    cleanup_conditionals(cstack, CSF_TRY, FALSE);

	    if (cstack->cs_idx >= 0
			       && (cstack->cs_flags[cstack->cs_idx] & CSF_TRY))
	    {
		// Variables declared in the previous block can no longer be
		// used.
		leave_block(cstack);
		enter_block(cstack);
	    }

	    /*
	     * Make did_emsg, got_int, did_throw pending.  If set, they overrule
	     * a pending ":continue", ":break", ":return", or ":finish".  Then
	     * we have particularly to discard a pending return value (as done
	     * by the call to cleanup_conditionals() above when did_emsg or
	     * got_int is set).  The pending values are restored by the
	     * ":endtry", except if there is a new error, interrupt, exception,
	     * ":continue", ":break", ":return", or ":finish" in the following
	     * finally clause.  A missing ":endwhile", ":endfor" or ":endif"
	     * detected here is treated as if did_emsg and did_throw had
	     * already been set, respectively in case that the error is not
	     * converted to an exception, did_throw had already been unset.
	     * We must not set did_emsg here since that would suppress the
	     * error message.
	     */
	    if (pending == CSTP_ERROR || did_emsg || got_int || did_throw)
	    {
		if (cstack->cs_pending[cstack->cs_idx] == CSTP_RETURN)
		{
		    report_discard_pending(CSTP_RETURN,
					   cstack->cs_rettv[cstack->cs_idx]);
		    discard_pending_return(cstack->cs_rettv[cstack->cs_idx]);
		}
		if (pending == CSTP_ERROR && !did_emsg)
		    pending |= (THROW_ON_ERROR) ? CSTP_THROW : 0;
		else
		    pending |= did_throw ? CSTP_THROW : 0;
		pending |= did_emsg  ? CSTP_ERROR     : 0;
		pending |= got_int   ? CSTP_INTERRUPT : 0;
		cstack->cs_pending[cstack->cs_idx] = pending;

		// It's mandatory that the current exception is stored in the
		// cstack so that it can be rethrown at the ":endtry" or be
		// discarded if the finally clause is left by a ":continue",
		// ":break", ":return", ":finish", error, interrupt, or another
		// exception.  When emsg() is called for a missing ":endif" or
		// a missing ":endwhile"/":endfor" detected here, the
		// exception will be discarded.
		if (did_throw && cstack->cs_exception[cstack->cs_idx]
							 != current_exception)
		    internal_error("ex_finally()");
	    }

	    /*
	     * Set CSL_HAD_FINA, so do_cmdline() will reset did_emsg,
	     * got_int, and did_throw and make the finally clause active.
	     * This will happen after emsg() has been called for a missing
	     * ":endif" or a missing ":endwhile"/":endfor" detected here, so
	     * that the following finally clause will be executed even then.
	     */
	    cstack->cs_lflags |= CSL_HAD_FINA;
	}
    }
}