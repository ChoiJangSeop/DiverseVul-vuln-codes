BGD_DECLARE(void *) gdImagePngPtrEx (gdImagePtr im, int *size, int level)
{
	void *rv;
	gdIOCtx *out = gdNewDynamicCtx (2048, NULL);
	if (out == NULL) return NULL;
	gdImagePngCtxEx (im, out, level);
	rv = gdDPExtractData (out, size);
	out->gd_free (out);
	return rv;
}