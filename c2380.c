isinsets(g, c)
register struct re_guts *g;
int c;
{
	register uch *col;
	register int i;
	register int ncols = (g->ncsets+(CHAR_BIT-1)) / CHAR_BIT;
	register unsigned uc = (unsigned char)c;

	for (i = 0, col = g->setbits; i < ncols; i++, col += g->csetsize)
		if (col[uc] != 0)
			return(1);
	return(0);
}