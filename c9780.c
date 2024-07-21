cmdloop(int top)
{
	union node *n;
	struct stackmark smark;
	int inter;
	int status = 0;
	int numeof = 0;

	TRACE(("cmdloop(%d) called\n", top));
	for (;;) {
		int skip;

		setstackmark(&smark);
		if (jobctl)
			showjobs(out2, SHOW_CHANGED);
		inter = 0;
		if (iflag && top) {
			inter++;
			chkmail();
		}
		n = parsecmd(inter);
		/* showtree(n); DEBUG */
		if (n == NEOF) {
			if (!top || numeof >= 50)
				break;
			if (!stoppedjobs()) {
				if (!Iflag) {
					if (iflag) {
						out2c('\n');
#ifdef FLUSHERR
						flushout(out2);
#endif
					}
					break;
				}
				out2str("\nUse \"exit\" to leave shell.\n");
			}
			numeof++;
		} else if (nflag == 0) {
			int i;

			job_warning = (job_warning == 2) ? 1 : 0;
			numeof = 0;
			i = evaltree(n, 0);
			if (n)
				status = i;
		}
		popstackmark(&smark);

		skip = evalskip;
		if (skip) {
			evalskip &= ~(SKIPFUNC | SKIPFUNCDEF);
			break;
		}
	}

	return status;
}