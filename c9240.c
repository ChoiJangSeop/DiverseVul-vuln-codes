static CACHE_BITMAP_V2_ORDER* update_read_cache_bitmap_v2_order(rdpUpdate* update, wStream* s,
                                                                BOOL compressed, UINT16 flags)
{
	BYTE bitsPerPixelId;
	CACHE_BITMAP_V2_ORDER* cache_bitmap_v2;

	if (!update || !s)
		return NULL;

	cache_bitmap_v2 = calloc(1, sizeof(CACHE_BITMAP_V2_ORDER));

	if (!cache_bitmap_v2)
		goto fail;

	cache_bitmap_v2->cacheId = flags & 0x0003;
	cache_bitmap_v2->flags = (flags & 0xFF80) >> 7;
	bitsPerPixelId = (flags & 0x0078) >> 3;
	cache_bitmap_v2->bitmapBpp = CBR2_BPP[bitsPerPixelId];

	if (cache_bitmap_v2->flags & CBR2_PERSISTENT_KEY_PRESENT)
	{
		if (Stream_GetRemainingLength(s) < 8)
			goto fail;

		Stream_Read_UINT32(s, cache_bitmap_v2->key1); /* key1 (4 bytes) */
		Stream_Read_UINT32(s, cache_bitmap_v2->key2); /* key2 (4 bytes) */
	}

	if (cache_bitmap_v2->flags & CBR2_HEIGHT_SAME_AS_WIDTH)
	{
		if (!update_read_2byte_unsigned(s, &cache_bitmap_v2->bitmapWidth)) /* bitmapWidth */
			goto fail;

		cache_bitmap_v2->bitmapHeight = cache_bitmap_v2->bitmapWidth;
	}
	else
	{
		if (!update_read_2byte_unsigned(s, &cache_bitmap_v2->bitmapWidth) || /* bitmapWidth */
		    !update_read_2byte_unsigned(s, &cache_bitmap_v2->bitmapHeight))  /* bitmapHeight */
			goto fail;
	}

	if (!update_read_4byte_unsigned(s, &cache_bitmap_v2->bitmapLength) || /* bitmapLength */
	    !update_read_2byte_unsigned(s, &cache_bitmap_v2->cacheIndex))     /* cacheIndex */
		goto fail;

	if (cache_bitmap_v2->flags & CBR2_DO_NOT_CACHE)
		cache_bitmap_v2->cacheIndex = BITMAP_CACHE_WAITING_LIST_INDEX;

	if (compressed)
	{
		if (!(cache_bitmap_v2->flags & CBR2_NO_BITMAP_COMPRESSION_HDR))
		{
			if (Stream_GetRemainingLength(s) < 8)
				goto fail;

			Stream_Read_UINT16(
			    s, cache_bitmap_v2->cbCompFirstRowSize); /* cbCompFirstRowSize (2 bytes) */
			Stream_Read_UINT16(
			    s, cache_bitmap_v2->cbCompMainBodySize);         /* cbCompMainBodySize (2 bytes) */
			Stream_Read_UINT16(s, cache_bitmap_v2->cbScanWidth); /* cbScanWidth (2 bytes) */
			Stream_Read_UINT16(
			    s, cache_bitmap_v2->cbUncompressedSize); /* cbUncompressedSize (2 bytes) */
			cache_bitmap_v2->bitmapLength = cache_bitmap_v2->cbCompMainBodySize;
		}
	}

	if (cache_bitmap_v2->bitmapLength == 0)
		goto fail;

	if (Stream_GetRemainingLength(s) < cache_bitmap_v2->bitmapLength)
		goto fail;

	if (cache_bitmap_v2->bitmapLength == 0)
		goto fail;

	cache_bitmap_v2->bitmapDataStream = malloc(cache_bitmap_v2->bitmapLength);

	if (!cache_bitmap_v2->bitmapDataStream)
		goto fail;

	Stream_Read(s, cache_bitmap_v2->bitmapDataStream, cache_bitmap_v2->bitmapLength);
	cache_bitmap_v2->compressed = compressed;
	return cache_bitmap_v2;
fail:
	free_cache_bitmap_v2_order(update->context, cache_bitmap_v2);
	return NULL;
}