static int jas_cmpxformseq_resize(jas_cmpxformseq_t *pxformseq, int n)
{
	jas_cmpxform_t **p;
	assert(n >= pxformseq->numpxforms);
	p = (!pxformseq->pxforms) ? jas_malloc(n * sizeof(jas_cmpxform_t *)) :
	  jas_realloc(pxformseq->pxforms, n * sizeof(jas_cmpxform_t *));
	if (!p) {
		return -1;
	}
	pxformseq->pxforms = p;
	pxformseq->maxpxforms = n;
	return 0;
}