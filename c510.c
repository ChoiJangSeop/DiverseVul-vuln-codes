int	special(DviContext *dvi, int opcode)
{
	char	*s;
	Int32	arg;
	
	arg = dugetn(dvi, opcode - DVI_XXX1 + 1);
	s = mdvi_malloc(arg + 1);
	dread(dvi, s, arg);
	s[arg] = 0;
	mdvi_do_special(dvi, s);
	SHOWCMD((dvi, "XXXX", opcode - DVI_XXX1 + 1,
		"[%s]", s));
	mdvi_free(s);
	return 0;
}