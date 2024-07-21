batchCopyElem(batch_obj_t *pDest, batch_obj_t *pSrc) {
	memcpy(pDest, pSrc, sizeof(batch_obj_t));
}