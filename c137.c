static int jas_iccprof_gettagtab(jas_stream_t *in, jas_icctagtab_t *tagtab)
{
	int i;
	jas_icctagtabent_t *tagtabent;

	if (tagtab->ents) {
		jas_free(tagtab->ents);
		tagtab->ents = 0;
	}
	if (jas_iccgetuint32(in, &tagtab->numents))
		goto error;
	if (!(tagtab->ents = jas_malloc(tagtab->numents *
	  sizeof(jas_icctagtabent_t))))
		goto error;
	tagtabent = tagtab->ents;
	for (i = 0; i < JAS_CAST(long, tagtab->numents); ++i) {
		if (jas_iccgetuint32(in, &tagtabent->tag) ||
		jas_iccgetuint32(in, &tagtabent->off) ||
		jas_iccgetuint32(in, &tagtabent->len))
			goto error;
		++tagtabent;
	}
	return 0;
error:
	if (tagtab->ents) {
		jas_free(tagtab->ents);
		tagtab->ents = 0;
	}
	return -1;
}