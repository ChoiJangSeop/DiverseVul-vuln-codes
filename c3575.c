jas_iccprof_t *jas_iccprof_load(jas_stream_t *in)
{
	jas_iccprof_t *prof;
	int numtags;
	long curoff;
	long reloff;
	long prevoff;
	jas_iccsig_t type;
	jas_iccattrval_t *attrval;
	jas_iccattrval_t *prevattrval;
	jas_icctagtabent_t *tagtabent;
	jas_iccattrvalinfo_t *attrvalinfo;
	int i;
	int len;

	prof = 0;
	attrval = 0;

	if (!(prof = jas_iccprof_create())) {
		goto error;
	}

	if (jas_iccprof_readhdr(in, &prof->hdr)) {
		jas_eprintf("cannot get header\n");
		goto error;
	}
	if (jas_iccprof_gettagtab(in, &prof->tagtab)) {
		jas_eprintf("cannot get tab table\n");
		goto error;
	}
	jas_iccprof_sorttagtab(&prof->tagtab);

	numtags = prof->tagtab.numents;
	curoff = JAS_ICC_HDRLEN + 4 + 12 * numtags;
	prevoff = 0;
	prevattrval = 0;
	for (i = 0; i < numtags; ++i) {
		tagtabent = &prof->tagtab.ents[i];
		if (tagtabent->off == JAS_CAST(jas_iccuint32_t, prevoff)) {
			if (prevattrval) {
				if (!(attrval = jas_iccattrval_clone(prevattrval)))
					goto error;
				if (jas_iccprof_setattr(prof, tagtabent->tag, attrval))
					goto error;
				jas_iccattrval_destroy(attrval);
			} else {
#if 0
				jas_eprintf("warning: skipping unknown tag type\n");
#endif
			}
			continue;
		}
		reloff = tagtabent->off - curoff;
		if (reloff > 0) {
			if (jas_stream_gobble(in, reloff) != reloff)
				goto error;
			curoff += reloff;
		} else if (reloff < 0) {
			/* This should never happen since we read the tagged
			element data in a single pass. */
			abort();
		}
		prevoff = curoff;
		if (jas_iccgetuint32(in, &type)) {
			goto error;
		}
		if (jas_stream_gobble(in, 4) != 4) {
			goto error;
		}
		curoff += 8;
		if (!(attrvalinfo = jas_iccattrvalinfo_lookup(type))) {
#if 0
			jas_eprintf("warning: skipping unknown tag type\n");
#endif
			prevattrval = 0;
			continue;
		}
		if (!(attrval = jas_iccattrval_create(type))) {
			goto error;
		}
		len = tagtabent->len - 8;
		if ((*attrval->ops->input)(attrval, in, len)) {
			goto error;
		}
		curoff += len;
		if (jas_iccprof_setattr(prof, tagtabent->tag, attrval)) {
			goto error;
		}
		prevattrval = attrval; /* This is correct, but slimey. */
		jas_iccattrval_destroy(attrval);
		attrval = 0;
	}

	return prof;

error:
	if (prof)
		jas_iccprof_destroy(prof);
	if (attrval)
		jas_iccattrval_destroy(attrval);
	return 0;
}