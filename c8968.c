static BOOL update_read_multi_opaque_rect_order(wStream* s, const ORDER_INFO* orderInfo,
                                                MULTI_OPAQUE_RECT_ORDER* multi_opaque_rect)
{
	BYTE byte;
	ORDER_FIELD_COORD(1, multi_opaque_rect->nLeftRect);
	ORDER_FIELD_COORD(2, multi_opaque_rect->nTopRect);
	ORDER_FIELD_COORD(3, multi_opaque_rect->nWidth);
	ORDER_FIELD_COORD(4, multi_opaque_rect->nHeight);

	if (orderInfo->fieldFlags & ORDER_FIELD_05)
	{
		if (Stream_GetRemainingLength(s) < 1)
			return FALSE;

		Stream_Read_UINT8(s, byte);
		multi_opaque_rect->color = (multi_opaque_rect->color & 0x00FFFF00) | ((UINT32)byte);
	}

	if (orderInfo->fieldFlags & ORDER_FIELD_06)
	{
		if (Stream_GetRemainingLength(s) < 1)
			return FALSE;

		Stream_Read_UINT8(s, byte);
		multi_opaque_rect->color = (multi_opaque_rect->color & 0x00FF00FF) | ((UINT32)byte << 8);
	}

	if (orderInfo->fieldFlags & ORDER_FIELD_07)
	{
		if (Stream_GetRemainingLength(s) < 1)
			return FALSE;

		Stream_Read_UINT8(s, byte);
		multi_opaque_rect->color = (multi_opaque_rect->color & 0x0000FFFF) | ((UINT32)byte << 16);
	}

	ORDER_FIELD_BYTE(8, multi_opaque_rect->numRectangles);

	if (orderInfo->fieldFlags & ORDER_FIELD_09)
	{
		if (Stream_GetRemainingLength(s) < 2)
			return FALSE;

		Stream_Read_UINT16(s, multi_opaque_rect->cbData);
		return update_read_delta_rects(s, multi_opaque_rect->rectangles,
		                               multi_opaque_rect->numRectangles);
	}

	return TRUE;
}