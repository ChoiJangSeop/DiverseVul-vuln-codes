Reprog *regcompx(void *(*alloc)(void *ctx, void *p, int n), void *ctx,
	const char *pattern, int cflags, const char **errorp)
{
	struct cstate g;
	Renode *node;
	Reinst *split, *jump;
	int i, n;

	g.pstart = NULL;
	g.prog = NULL;

	if (setjmp(g.kaboom)) {
		if (errorp) *errorp = g.error;
		alloc(ctx, g.pstart, 0);
		alloc(ctx, g.prog, 0);
		return NULL;
	}

	g.prog = alloc(ctx, NULL, sizeof (Reprog));
	if (!g.prog)
		die(&g, "cannot allocate regular expression");
	n = strlen(pattern) * 2;
	if (n > REG_MAXPROG)
		die(&g, "program too large");
	if (n > 0) {
		g.pstart = g.pend = alloc(ctx, NULL, sizeof (Renode) * n);
		if (!g.pstart)
			die(&g, "cannot allocate regular expression parse list");
	}

	g.source = pattern;
	g.ncclass = 0;
	g.nsub = 1;
	for (i = 0; i < REG_MAXSUB; ++i)
		g.sub[i] = 0;

	g.prog->flags = cflags;

	next(&g);
	node = parsealt(&g);
	if (g.lookahead == ')')
		die(&g, "unmatched ')'");
	if (g.lookahead != EOF)
		die(&g, "syntax error");

#ifdef TEST
	dumpnode(node);
	putchar('\n');
#endif

	n = 6 + count(&g, node);
	if (n < 0 || n > REG_MAXPROG)
		die(&g, "program too large");

	g.prog->nsub = g.nsub;
	g.prog->start = g.prog->end = alloc(ctx, NULL, n * sizeof (Reinst));
	if (!g.prog->start)
		die(&g, "cannot allocate regular expression instruction list");

	split = emit(g.prog, I_SPLIT);
	split->x = split + 3;
	split->y = split + 1;
	emit(g.prog, I_ANYNL);
	jump = emit(g.prog, I_JUMP);
	jump->x = split;
	emit(g.prog, I_LPAR);
	compile(g.prog, node);
	emit(g.prog, I_RPAR);
	emit(g.prog, I_END);

#ifdef TEST
	dumpprog(g.prog);
#endif

	alloc(ctx, g.pstart, 0);

	if (errorp) *errorp = NULL;
	return g.prog;
}