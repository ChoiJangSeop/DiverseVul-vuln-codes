static int jas_iccattrtab_resize(jas_iccattrtab_t *tab, int maxents)
{
	jas_iccattr_t *newattrs;
	assert(maxents >= tab->numattrs);
	newattrs = tab->attrs ? jas_realloc(tab->attrs, maxents *
	  sizeof(jas_iccattr_t)) : jas_malloc(maxents * sizeof(jas_iccattr_t));
	if (!newattrs)
		return -1;
	tab->attrs = newattrs;
	tab->maxattrs = maxents;
	return 0;
}