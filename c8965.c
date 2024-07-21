static CACHE_BITMAP_ORDER* update_read_cache_bitmap_order(rdpUpdate* update, wStream* s,
                                                          BOOL compressed, UINT16 flags)
{
	CACHE_BITMAP_ORDER* cache_bitmap;

	if (!update || !s)
		return NULL;

	cache_bitmap = calloc(1, sizeof(CACHE_BITMAP_ORDER));

	if (!cache_bitmap)
		goto fail;

	if (Stream_GetRemainingLength(s) < 9)
		goto fail;

	Stream_Read_UINT8(s, cache_bitmap->cacheId);      /* cacheId (1 byte) */
	Stream_Seek_UINT8(s);                             /* pad1Octet (1 byte) */
	Stream_Read_UINT8(s, cache_bitmap->bitmapWidth);  /* bitmapWidth (1 byte) */
	Stream_Read_UINT8(s, cache_bitmap->bitmapHeight); /* bitmapHeight (1 byte) */
	Stream_Read_UINT8(s, cache_bitmap->bitmapBpp);    /* bitmapBpp (1 byte) */

	if ((cache_bitmap->bitmapBpp < 1) || (cache_bitmap->bitmapBpp > 32))
	{
		WLog_Print(update->log, WLOG_ERROR, "invalid bitmap bpp %" PRIu32 "",
		           cache_bitmap->bitmapBpp);
		goto fail;
	}

	Stream_Read_UINT16(s, cache_bitmap->bitmapLength); /* bitmapLength (2 bytes) */
	Stream_Read_UINT16(s, cache_bitmap->cacheIndex);   /* cacheIndex (2 bytes) */

	if (compressed)
	{
		if ((flags & NO_BITMAP_COMPRESSION_HDR) == 0)
		{
			BYTE* bitmapComprHdr = (BYTE*)&(cache_bitmap->bitmapComprHdr);

			if (Stream_GetRemainingLength(s) < 8)
				goto fail;

			Stream_Read(s, bitmapComprHdr, 8); /* bitmapComprHdr (8 bytes) */
			cache_bitmap->bitmapLength -= 8;
		}
	}

	if (Stream_GetRemainingLength(s) < cache_bitmap->bitmapLength)
		goto fail;

	cache_bitmap->bitmapDataStream = malloc(cache_bitmap->bitmapLength);

	if (!cache_bitmap->bitmapDataStream)
		goto fail;

	Stream_Read(s, cache_bitmap->bitmapDataStream, cache_bitmap->bitmapLength);
	cache_bitmap->compressed = compressed;
	return cache_bitmap;
fail:
	free_cache_bitmap_order(update->context, cache_bitmap);
	return NULL;
}