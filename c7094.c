BGD_DECLARE(void *) gdImageJpegPtr(gdImagePtr im, int *size, int quality)
{
	void *rv;
	gdIOCtx *out = gdNewDynamicCtx(2048, NULL);
	if (out == NULL) return NULL;
	gdImageJpegCtx(im, out, quality);
	rv = gdDPExtractData(out, size);
	out->gd_free(out);
	return rv;
}