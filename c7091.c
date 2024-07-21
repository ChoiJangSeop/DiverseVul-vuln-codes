BGD_DECLARE(void *) gdImageWBMPPtr(gdImagePtr im, int *size, int fg)
{
	void *rv;
	gdIOCtx *out = gdNewDynamicCtx(2048, NULL);
	if (out == NULL) return NULL;
	gdImageWBMPCtx(im, fg, out);
	rv = gdDPExtractData(out, size);
	out->gd_free(out);
	return rv;
}