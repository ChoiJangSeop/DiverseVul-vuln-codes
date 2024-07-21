int jas_iccprof_save(jas_iccprof_t *prof, jas_stream_t *out)
{
	long curoff;
	long reloff;
	long newoff;
	int i;
	int j;
	jas_icctagtabent_t *tagtabent;
	jas_icctagtabent_t *sharedtagtabent;
	jas_icctagtabent_t *tmptagtabent;
	jas_iccuint32_t attrname;
	jas_iccattrval_t *attrval;
	jas_icctagtab_t *tagtab;

	tagtab = &prof->tagtab;
	if (!(tagtab->ents = jas_malloc(prof->attrtab->numattrs *
	  sizeof(jas_icctagtabent_t))))
		goto error;
	tagtab->numents = prof->attrtab->numattrs;
	curoff = JAS_ICC_HDRLEN + 4 + 12 * tagtab->numents;
	for (i = 0; i < JAS_CAST(int, tagtab->numents); ++i) {
		tagtabent = &tagtab->ents[i];
		if (jas_iccattrtab_get(prof->attrtab, i, &attrname, &attrval))
			goto error;
		assert(attrval->ops->output);
		tagtabent->tag = attrname;
		tagtabent->data = &attrval->data;
		sharedtagtabent = 0;
		for (j = 0; j < i; ++j) {
			tmptagtabent = &tagtab->ents[j];
			if (tagtabent->data == tmptagtabent->data) {
				sharedtagtabent = tmptagtabent;
				break;
			}
		}
		if (sharedtagtabent) {
			tagtabent->off = sharedtagtabent->off;
			tagtabent->len = sharedtagtabent->len;
			tagtabent->first = sharedtagtabent;
		} else {
			tagtabent->off = curoff;
			tagtabent->len = (*attrval->ops->getsize)(attrval) + 8;
			tagtabent->first = 0;
			if (i < JAS_CAST(int, tagtab->numents - 1)) {
				curoff = jas_iccpadtomult(curoff + tagtabent->len, 4);
			} else {
				curoff += tagtabent->len;
			}
		}
		jas_iccattrval_destroy(attrval);
	}
	prof->hdr.size = curoff;
	if (jas_iccprof_writehdr(out, &prof->hdr))
		goto error;
	if (jas_iccprof_puttagtab(out, &prof->tagtab))
		goto error;
	curoff = JAS_ICC_HDRLEN + 4 + 12 * tagtab->numents;
	for (i = 0; i < JAS_CAST(int, tagtab->numents);) {
		tagtabent = &tagtab->ents[i];
		assert(curoff == JAS_CAST(long, tagtabent->off));
		if (jas_iccattrtab_get(prof->attrtab, i, &attrname, &attrval))
			goto error;
		if (jas_iccputuint32(out, attrval->type) || jas_stream_pad(out,
		  4, 0) != 4)
			goto error;
		if ((*attrval->ops->output)(attrval, out))
			goto error;
		jas_iccattrval_destroy(attrval);
		curoff += tagtabent->len;
		++i;
		while (i < JAS_CAST(int, tagtab->numents) &&
		  tagtab->ents[i].first)
			++i;
		newoff = (i < JAS_CAST(int, tagtab->numents)) ?
		  tagtab->ents[i].off : prof->hdr.size;
		reloff = newoff - curoff;
		assert(reloff >= 0);
		if (reloff > 0) {
			if (jas_stream_pad(out, reloff, 0) != reloff)
				goto error;
			curoff += reloff;
		}
	}	
	return 0;
error:
	/* XXX - need to free some resources here */
	return -1;
}