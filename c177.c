int jpc_pchglist_insert(jpc_pchglist_t *pchglist, int pchgno, jpc_pchg_t *pchg)
{
	int i;
	int newmaxpchgs;
	jpc_pchg_t **newpchgs;
	if (pchgno < 0) {
		pchgno = pchglist->numpchgs;
	}
	if (pchglist->numpchgs >= pchglist->maxpchgs) {
		newmaxpchgs = pchglist->maxpchgs + 128;
		if (!(newpchgs = jas_realloc(pchglist->pchgs, newmaxpchgs * sizeof(jpc_pchg_t *)))) {
			return -1;
		}
		pchglist->maxpchgs = newmaxpchgs;
		pchglist->pchgs = newpchgs;
	}
	for (i = pchglist->numpchgs; i > pchgno; --i) {
		pchglist->pchgs[i] = pchglist->pchgs[i - 1];
	}
	pchglist->pchgs[pchgno] = pchg;
	++pchglist->numpchgs;
	return 0;
}