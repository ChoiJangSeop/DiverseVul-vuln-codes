static BOOL update_read_multi_patblt_order(wStream* s, const ORDER_INFO* orderInfo,
                                           MULTI_PATBLT_ORDER* multi_patblt)
{
	ORDER_FIELD_COORD(1, multi_patblt->nLeftRect);
	ORDER_FIELD_COORD(2, multi_patblt->nTopRect);
	ORDER_FIELD_COORD(3, multi_patblt->nWidth);
	ORDER_FIELD_COORD(4, multi_patblt->nHeight);
	ORDER_FIELD_BYTE(5, multi_patblt->bRop);
	ORDER_FIELD_COLOR(orderInfo, s, 6, &multi_patblt->backColor);
	ORDER_FIELD_COLOR(orderInfo, s, 7, &multi_patblt->foreColor);

	if (!update_read_brush(s, &multi_patblt->brush, orderInfo->fieldFlags >> 7))
		return FALSE;

	ORDER_FIELD_BYTE(13, multi_patblt->numRectangles);

	if (orderInfo->fieldFlags & ORDER_FIELD_14)
	{
		if (Stream_GetRemainingLength(s) < 2)
			return FALSE;

		Stream_Read_UINT16(s, multi_patblt->cbData);

		if (!update_read_delta_rects(s, multi_patblt->rectangles, multi_patblt->numRectangles))
			return FALSE;
	}

	return TRUE;
}