evaltree(union node *n, int flags)
{
	int checkexit = 0;
	int (*evalfn)(union node *, int);
	struct stackmark smark;
	unsigned isor;
	int status = 0;

	setstackmark(&smark);

	if (n == NULL) {
		TRACE(("evaltree(NULL) called\n"));
		goto out;
	}

	dotrap();

#ifndef SMALL
	displayhist = 1;	/* show history substitutions done with fc */
#endif
	TRACE(("pid %d, evaltree(%p: %d, %d) called\n",
	    getpid(), n, n->type, flags));
	switch (n->type) {
	default:
#ifdef DEBUG
		out1fmt("Node type = %d\n", n->type);
#ifndef USE_GLIBC_STDIO
		flushout(out1);
#endif
		break;
#endif
	case NNOT:
		status = !evaltree(n->nnot.com, EV_TESTED);
		goto setstatus;
	case NREDIR:
		errlinno = lineno = n->nredir.linno;
		if (funcline)
			lineno -= funcline - 1;
		expredir(n->nredir.redirect);
		pushredir(n->nredir.redirect);
		status = redirectsafe(n->nredir.redirect, REDIR_PUSH) ?:
			 evaltree(n->nredir.n, flags & EV_TESTED);
		if (n->nredir.redirect)
			popredir(0);
		goto setstatus;
	case NCMD:
#ifdef notyet
		if (eflag && !(flags & EV_TESTED))
			checkexit = ~0;
		status = evalcommand(n, flags, (struct backcmd *)NULL);
		goto setstatus;
#else
		evalfn = evalcommand;
checkexit:
		if (eflag && !(flags & EV_TESTED))
			checkexit = ~0;
		goto calleval;
#endif
	case NFOR:
		evalfn = evalfor;
		goto calleval;
	case NWHILE:
	case NUNTIL:
		evalfn = evalloop;
		goto calleval;
	case NSUBSHELL:
	case NBACKGND:
		evalfn = evalsubshell;
		goto checkexit;
	case NPIPE:
		evalfn = evalpipe;
		goto checkexit;
	case NCASE:
		evalfn = evalcase;
		goto calleval;
	case NAND:
	case NOR:
	case NSEMI:
#if NAND + 1 != NOR
#error NAND + 1 != NOR
#endif
#if NOR + 1 != NSEMI
#error NOR + 1 != NSEMI
#endif
		isor = n->type - NAND;
		status = evaltree(n->nbinary.ch1,
				  (flags | ((isor >> 1) - 1)) & EV_TESTED);
		if ((!status) == isor || evalskip)
			break;
		n = n->nbinary.ch2;
evaln:
		evalfn = evaltree;
calleval:
		status = evalfn(n, flags);
		goto setstatus;
	case NIF:
		status = evaltree(n->nif.test, EV_TESTED);
		if (evalskip)
			break;
		if (!status) {
			n = n->nif.ifpart;
			goto evaln;
		} else if (n->nif.elsepart) {
			n = n->nif.elsepart;
			goto evaln;
		}
		status = 0;
		goto setstatus;
	case NDEFUN:
		defun(n);
setstatus:
		exitstatus = status;
		break;
	}
out:
	dotrap();

	if (checkexit & status)
		goto exexit;

	if (flags & EV_EXIT) {
exexit:
		exraise(EXEND);
	}

	popstackmark(&smark);

	return exitstatus;
}