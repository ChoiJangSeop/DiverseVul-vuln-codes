static BOOL update_read_multi_scrblt_order(wStream* s, const ORDER_INFO* orderInfo,
                                           MULTI_SCRBLT_ORDER* multi_scrblt)
{
	ORDER_FIELD_COORD(1, multi_scrblt->nLeftRect);
	ORDER_FIELD_COORD(2, multi_scrblt->nTopRect);
	ORDER_FIELD_COORD(3, multi_scrblt->nWidth);
	ORDER_FIELD_COORD(4, multi_scrblt->nHeight);
	ORDER_FIELD_BYTE(5, multi_scrblt->bRop);
	ORDER_FIELD_COORD(6, multi_scrblt->nXSrc);
	ORDER_FIELD_COORD(7, multi_scrblt->nYSrc);
	ORDER_FIELD_BYTE(8, multi_scrblt->numRectangles);

	if (orderInfo->fieldFlags & ORDER_FIELD_09)
	{
		if (Stream_GetRemainingLength(s) < 2)
			return FALSE;

		Stream_Read_UINT16(s, multi_scrblt->cbData);
		return update_read_delta_rects(s, multi_scrblt->rectangles, multi_scrblt->numRectangles);
	}

	return TRUE;
}