compile_lock_unlock(
    lval_T  *lvp,
    char_u  *name_end,
    exarg_T *eap,
    int	    deep,
    void    *coookie)
{
    cctx_T	*cctx = coookie;
    int		cc = *name_end;
    char_u	*p = lvp->ll_name;
    int		ret = OK;
    size_t	len;
    char_u	*buf;
    isntype_T	isn = ISN_EXEC;

    if (cctx->ctx_skip == SKIP_YES)
	return OK;

    // Cannot use :lockvar and :unlockvar on local variables.
    if (p[1] != ':')
    {
	char_u *end = find_name_end(p, NULL, NULL, FNE_CHECK_START);

	if (lookup_local(p, end - p, NULL, cctx) == OK)
	{
	    char_u *s = p;

	    if (*end != '.' && *end != '[')
	    {
		emsg(_(e_cannot_lock_unlock_local_variable));
		return FAIL;
	    }

	    // For "d.member" put the local variable on the stack, it will be
	    // passed to ex_lockvar() indirectly.
	    if (compile_load(&s, end, cctx, FALSE, FALSE) == FAIL)
		return FAIL;
	    isn = ISN_LOCKUNLOCK;
	}
    }

    // Checking is done at runtime.
    *name_end = NUL;
    len = name_end - p + 20;
    buf = alloc(len);
    if (buf == NULL)
	ret = FAIL;
    else
    {
	char *cmd = eap->cmdidx == CMD_lockvar ? "lockvar" : "unlockvar";

	if (deep < 0)
	    vim_snprintf((char *)buf, len, "%s! %s", cmd, p);
	else
	    vim_snprintf((char *)buf, len, "%s %d %s", cmd, deep, p);
	ret = generate_EXEC_copy(cctx, isn, buf);

	vim_free(buf);
	*name_end = cc;
    }
    return ret;
}