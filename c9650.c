int pmixp_coll_tree_unpack(Buf buf, pmixp_coll_type_t *type,
			   int *nodeid, pmixp_proc_t **r, size_t *nr)
{
	pmixp_proc_t *procs = NULL;
	uint32_t nprocs = 0;
	uint32_t tmp;
	int i, rc;

	/* 1. extract the type of collective */
	if (SLURM_SUCCESS != (rc = unpack32(&tmp, buf))) {
		PMIXP_ERROR("Cannot unpack collective type");
		return rc;
	}
	*type = tmp;

	/* 2. get the number of ranges */
	if (SLURM_SUCCESS != (rc = unpack32(&nprocs, buf))) {
		PMIXP_ERROR("Cannot unpack collective type");
		return rc;
	}
	*nr = nprocs;

	procs = xmalloc(sizeof(pmixp_proc_t) * nprocs);
	*r = procs;

	for (i = 0; i < (int)nprocs; i++) {
		/* 3. get namespace/rank of particular process */
		rc = unpackmem(procs[i].nspace, &tmp, buf);
		if (SLURM_SUCCESS != rc) {
			PMIXP_ERROR("Cannot unpack namespace for process #%d",
				    i);
			return rc;
		}
		procs[i].nspace[tmp] = '\0';

		unsigned int tmp;
		rc = unpack32(&tmp, buf);
		procs[i].rank = tmp;
		if (SLURM_SUCCESS != rc) {
			PMIXP_ERROR("Cannot unpack ranks for process #%d, nsp=%s",
				    i, procs[i].nspace);
			return rc;
		}
	}
	return SLURM_SUCCESS;
}