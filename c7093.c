BGD_DECLARE(void *) gdImageGifPtr(gdImagePtr im, int *size)
{
	void *rv;
	gdIOCtx *out = gdNewDynamicCtx(2048, NULL);
	if (out == NULL) return NULL;
	gdImageGifCtx(im, out);
	rv = gdDPExtractData(out, size);
	out->gd_free(out);
	return rv;
}