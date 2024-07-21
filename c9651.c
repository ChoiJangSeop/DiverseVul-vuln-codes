int pmixp_coll_ring_unpack(Buf buf, pmixp_coll_type_t *type,
			   pmixp_coll_ring_msg_hdr_t *ring_hdr,
			   pmixp_proc_t **r, size_t *nr)
{
	pmixp_proc_t *procs = NULL;
	uint32_t nprocs = 0;
	uint32_t tmp;
	int rc, i;

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

	/* 3. get namespace/rank of particular process */
	for (i = 0; i < (int)nprocs; i++) {
		rc = unpackmem(procs[i].nspace, &tmp, buf);
		if (SLURM_SUCCESS != rc) {
			PMIXP_ERROR("Cannot unpack namespace for process #%d",
				    i);
			return rc;
		}
		procs[i].nspace[tmp] = '\0';

		rc = unpack32(&tmp, buf);
		procs[i].rank = tmp;
		if (SLURM_SUCCESS != rc) {
			PMIXP_ERROR("Cannot unpack ranks for process #%d, nsp=%s",
				    i, procs[i].nspace);
			return rc;
		}
	}

	/* 4. extract the ring info */
	if (SLURM_SUCCESS != (rc = unpackmem((char *)ring_hdr, &tmp, buf))) {
		PMIXP_ERROR("Cannot unpack ring info");
		return rc;
	}

	return SLURM_SUCCESS;
}