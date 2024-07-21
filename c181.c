int jpc_ppxstab_grow(jpc_ppxstab_t *tab, int maxents)
{
	jpc_ppxstabent_t **newents;
	if (tab->maxents < maxents) {
		newents = (tab->ents) ? jas_realloc(tab->ents, maxents *
		  sizeof(jpc_ppxstabent_t *)) : jas_malloc(maxents * sizeof(jpc_ppxstabent_t *));
		if (!newents) {
			return -1;
		}
		tab->ents = newents;
		tab->maxents = maxents;
	}
	return 0;
}