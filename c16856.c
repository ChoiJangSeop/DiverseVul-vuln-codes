OJPEGDecode(TIFF* tif, uint8* buf, tmsize_t cc, uint16 s)
{
	OJPEGState* sp=(OJPEGState*)tif->tif_data;
	(void)s;
	if (sp->libjpeg_jpeg_query_style==0)
	{
		if (OJPEGDecodeRaw(tif,buf,cc)==0)
			return(0);
	}
	else
	{
		if (OJPEGDecodeScanlines(tif,buf,cc)==0)
			return(0);
	}
	return(1);
}