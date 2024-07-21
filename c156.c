static int jas_icccurv_input(jas_iccattrval_t *attrval, jas_stream_t *in,
  int cnt)
{
	jas_icccurv_t *curv = &attrval->data.curv;
	unsigned int i;

	curv->numents = 0;
	curv->ents = 0;

	if (jas_iccgetuint32(in, &curv->numents))
		goto error;
	if (!(curv->ents = jas_malloc(curv->numents * sizeof(jas_iccuint16_t))))
		goto error;
	for (i = 0; i < curv->numents; ++i) {
		if (jas_iccgetuint16(in, &curv->ents[i]))
			goto error;
	}

	if (JAS_CAST(int, 4 + 2 * curv->numents) != cnt)
		goto error;
	return 0;

error:
	jas_icccurv_destroy(attrval);
	return -1;
}