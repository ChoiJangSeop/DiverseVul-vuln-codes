static BOOL update_read_bitmap_data(rdpUpdate* update, wStream* s, BITMAP_DATA* bitmapData)
{
	WINPR_UNUSED(update);
	if (Stream_GetRemainingLength(s) < 18)
		return FALSE;

	Stream_Read_UINT16(s, bitmapData->destLeft);
	Stream_Read_UINT16(s, bitmapData->destTop);
	Stream_Read_UINT16(s, bitmapData->destRight);
	Stream_Read_UINT16(s, bitmapData->destBottom);
	Stream_Read_UINT16(s, bitmapData->width);
	Stream_Read_UINT16(s, bitmapData->height);
	Stream_Read_UINT16(s, bitmapData->bitsPerPixel);
	Stream_Read_UINT16(s, bitmapData->flags);
	Stream_Read_UINT16(s, bitmapData->bitmapLength);

	if (bitmapData->flags & BITMAP_COMPRESSION)
	{
		if (!(bitmapData->flags & NO_BITMAP_COMPRESSION_HDR))
		{
			Stream_Read_UINT16(s,
			                   bitmapData->cbCompFirstRowSize); /* cbCompFirstRowSize (2 bytes) */
			Stream_Read_UINT16(s,
			                   bitmapData->cbCompMainBodySize); /* cbCompMainBodySize (2 bytes) */
			Stream_Read_UINT16(s, bitmapData->cbScanWidth);     /* cbScanWidth (2 bytes) */
			Stream_Read_UINT16(s,
			                   bitmapData->cbUncompressedSize); /* cbUncompressedSize (2 bytes) */
			bitmapData->bitmapLength = bitmapData->cbCompMainBodySize;
		}

		bitmapData->compressed = TRUE;
	}
	else
		bitmapData->compressed = FALSE;

	if (Stream_GetRemainingLength(s) < bitmapData->bitmapLength)
		return FALSE;

	if (bitmapData->bitmapLength > 0)
	{
		bitmapData->bitmapDataStream = malloc(bitmapData->bitmapLength);

		if (!bitmapData->bitmapDataStream)
			return FALSE;

		memcpy(bitmapData->bitmapDataStream, Stream_Pointer(s), bitmapData->bitmapLength);
		Stream_Seek(s, bitmapData->bitmapLength);
	}

	return TRUE;
}