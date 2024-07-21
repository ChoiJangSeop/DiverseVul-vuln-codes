BOOL update_write_cache_brush_order(wStream* s, const CACHE_BRUSH_ORDER* cache_brush, UINT16* flags)
{
	int i;
	BYTE iBitmapFormat;
	BOOL compressed = FALSE;

	if (!Stream_EnsureRemainingCapacity(s,
	                                    update_approximate_cache_brush_order(cache_brush, flags)))
		return FALSE;

	iBitmapFormat = BPP_BMF[cache_brush->bpp];
	Stream_Write_UINT8(s, cache_brush->index);  /* cacheEntry (1 byte) */
	Stream_Write_UINT8(s, iBitmapFormat);       /* iBitmapFormat (1 byte) */
	Stream_Write_UINT8(s, cache_brush->cx);     /* cx (1 byte) */
	Stream_Write_UINT8(s, cache_brush->cy);     /* cy (1 byte) */
	Stream_Write_UINT8(s, cache_brush->style);  /* style (1 byte) */
	Stream_Write_UINT8(s, cache_brush->length); /* iBytes (1 byte) */

	if ((cache_brush->cx == 8) && (cache_brush->cy == 8))
	{
		if (cache_brush->bpp == 1)
		{
			if (cache_brush->length != 8)
			{
				WLog_ERR(TAG, "incompatible 1bpp brush of length:%" PRIu32 "", cache_brush->length);
				return FALSE;
			}

			for (i = 7; i >= 0; i--)
			{
				Stream_Write_UINT8(s, cache_brush->data[i]);
			}
		}
		else
		{
			if ((iBitmapFormat == BMF_8BPP) && (cache_brush->length == 20))
				compressed = TRUE;
			else if ((iBitmapFormat == BMF_16BPP) && (cache_brush->length == 24))
				compressed = TRUE;
			else if ((iBitmapFormat == BMF_32BPP) && (cache_brush->length == 32))
				compressed = TRUE;

			if (compressed != FALSE)
			{
				/* compressed brush */
				if (!update_compress_brush(s, cache_brush->data, cache_brush->bpp))
					return FALSE;
			}
			else
			{
				/* uncompressed brush */
				int scanline = (cache_brush->bpp / 8) * 8;

				for (i = 7; i >= 0; i--)
				{
					Stream_Write(s, &cache_brush->data[i * scanline], scanline);
				}
			}
		}
	}

	return TRUE;
}