static BOOL update_read_multi_dstblt_order(wStream* s, const ORDER_INFO* orderInfo,
                                           MULTI_DSTBLT_ORDER* multi_dstblt)
{
	ORDER_FIELD_COORD(1, multi_dstblt->nLeftRect);
	ORDER_FIELD_COORD(2, multi_dstblt->nTopRect);
	ORDER_FIELD_COORD(3, multi_dstblt->nWidth);
	ORDER_FIELD_COORD(4, multi_dstblt->nHeight);
	ORDER_FIELD_BYTE(5, multi_dstblt->bRop);
	ORDER_FIELD_BYTE(6, multi_dstblt->numRectangles);

	if (orderInfo->fieldFlags & ORDER_FIELD_07)
	{
		if (Stream_GetRemainingLength(s) < 2)
			return FALSE;

		Stream_Read_UINT16(s, multi_dstblt->cbData);
		return update_read_delta_rects(s, multi_dstblt->rectangles, multi_dstblt->numRectangles);
	}

	return TRUE;
}