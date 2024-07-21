BOOL update_write_cache_bitmap_v2_order(wStream* s, CACHE_BITMAP_V2_ORDER* cache_bitmap_v2,
                                        BOOL compressed, UINT16* flags)
{
	BYTE bitsPerPixelId;

	if (!Stream_EnsureRemainingCapacity(
	        s, update_approximate_cache_bitmap_v2_order(cache_bitmap_v2, compressed, flags)))
		return FALSE;

	bitsPerPixelId = BPP_CBR2[cache_bitmap_v2->bitmapBpp];
	*flags = (cache_bitmap_v2->cacheId & 0x0003) | (bitsPerPixelId << 3) |
	         ((cache_bitmap_v2->flags << 7) & 0xFF80);

	if (cache_bitmap_v2->flags & CBR2_PERSISTENT_KEY_PRESENT)
	{
		Stream_Write_UINT32(s, cache_bitmap_v2->key1); /* key1 (4 bytes) */
		Stream_Write_UINT32(s, cache_bitmap_v2->key2); /* key2 (4 bytes) */
	}

	if (cache_bitmap_v2->flags & CBR2_HEIGHT_SAME_AS_WIDTH)
	{
		if (!update_write_2byte_unsigned(s, cache_bitmap_v2->bitmapWidth)) /* bitmapWidth */
			return FALSE;
	}
	else
	{
		if (!update_write_2byte_unsigned(s, cache_bitmap_v2->bitmapWidth) || /* bitmapWidth */
		    !update_write_2byte_unsigned(s, cache_bitmap_v2->bitmapHeight))  /* bitmapHeight */
			return FALSE;
	}

	if (cache_bitmap_v2->flags & CBR2_DO_NOT_CACHE)
		cache_bitmap_v2->cacheIndex = BITMAP_CACHE_WAITING_LIST_INDEX;

	if (!update_write_4byte_unsigned(s, cache_bitmap_v2->bitmapLength) || /* bitmapLength */
	    !update_write_2byte_unsigned(s, cache_bitmap_v2->cacheIndex))     /* cacheIndex */
		return FALSE;

	if (compressed)
	{
		if (!(cache_bitmap_v2->flags & CBR2_NO_BITMAP_COMPRESSION_HDR))
		{
			Stream_Write_UINT16(
			    s, cache_bitmap_v2->cbCompFirstRowSize); /* cbCompFirstRowSize (2 bytes) */
			Stream_Write_UINT16(
			    s, cache_bitmap_v2->cbCompMainBodySize);          /* cbCompMainBodySize (2 bytes) */
			Stream_Write_UINT16(s, cache_bitmap_v2->cbScanWidth); /* cbScanWidth (2 bytes) */
			Stream_Write_UINT16(
			    s, cache_bitmap_v2->cbUncompressedSize); /* cbUncompressedSize (2 bytes) */
			cache_bitmap_v2->bitmapLength = cache_bitmap_v2->cbCompMainBodySize;
		}

		if (!Stream_EnsureRemainingCapacity(s, cache_bitmap_v2->bitmapLength))
			return FALSE;

		Stream_Write(s, cache_bitmap_v2->bitmapDataStream, cache_bitmap_v2->bitmapLength);
	}
	else
	{
		if (!Stream_EnsureRemainingCapacity(s, cache_bitmap_v2->bitmapLength))
			return FALSE;

		Stream_Write(s, cache_bitmap_v2->bitmapDataStream, cache_bitmap_v2->bitmapLength);
	}

	cache_bitmap_v2->compressed = compressed;
	return TRUE;
}