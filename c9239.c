static CACHE_BRUSH_ORDER* update_read_cache_brush_order(rdpUpdate* update, wStream* s, UINT16 flags)
{
	int i;
	BYTE iBitmapFormat;
	BOOL compressed = FALSE;
	CACHE_BRUSH_ORDER* cache_brush = calloc(1, sizeof(CACHE_BRUSH_ORDER));

	if (!cache_brush)
		goto fail;

	if (Stream_GetRemainingLength(s) < 6)
		goto fail;

	Stream_Read_UINT8(s, cache_brush->index); /* cacheEntry (1 byte) */
	Stream_Read_UINT8(s, iBitmapFormat);      /* iBitmapFormat (1 byte) */

	if (iBitmapFormat >= ARRAYSIZE(BMF_BPP))
		goto fail;

	cache_brush->bpp = BMF_BPP[iBitmapFormat];
	Stream_Read_UINT8(s, cache_brush->cx);     /* cx (1 byte) */
	Stream_Read_UINT8(s, cache_brush->cy);     /* cy (1 byte) */
	Stream_Read_UINT8(s, cache_brush->style);  /* style (1 byte) */
	Stream_Read_UINT8(s, cache_brush->length); /* iBytes (1 byte) */

	if ((cache_brush->cx == 8) && (cache_brush->cy == 8))
	{
		if (cache_brush->bpp == 1)
		{
			if (cache_brush->length != 8)
			{
				WLog_Print(update->log, WLOG_ERROR, "incompatible 1bpp brush of length:%" PRIu32 "",
				           cache_brush->length);
				goto fail;
			}

			/* rows are encoded in reverse order */
			if (Stream_GetRemainingLength(s) < 8)
				goto fail;

			for (i = 7; i >= 0; i--)
			{
				Stream_Read_UINT8(s, cache_brush->data[i]);
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
				if (!update_decompress_brush(s, cache_brush->data, sizeof(cache_brush->data),
				                             cache_brush->bpp))
					goto fail;
			}
			else
			{
				/* uncompressed brush */
				UINT32 scanline = (cache_brush->bpp / 8) * 8;

				if (Stream_GetRemainingLength(s) < scanline * 8)
					goto fail;

				for (i = 7; i >= 0; i--)
				{
					Stream_Read(s, &cache_brush->data[i * scanline], scanline);
				}
			}
		}
	}

	return cache_brush;
fail:
	free_cache_brush_order(update->context, cache_brush);
	return NULL;
}