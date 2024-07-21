static BOOL update_read_multi_draw_nine_grid_order(wStream* s, const ORDER_INFO* orderInfo,
                                                   MULTI_DRAW_NINE_GRID_ORDER* multi_draw_nine_grid)
{
	ORDER_FIELD_COORD(1, multi_draw_nine_grid->srcLeft);
	ORDER_FIELD_COORD(2, multi_draw_nine_grid->srcTop);
	ORDER_FIELD_COORD(3, multi_draw_nine_grid->srcRight);
	ORDER_FIELD_COORD(4, multi_draw_nine_grid->srcBottom);
	ORDER_FIELD_UINT16(5, multi_draw_nine_grid->bitmapId);
	ORDER_FIELD_BYTE(6, multi_draw_nine_grid->nDeltaEntries);

	if (orderInfo->fieldFlags & ORDER_FIELD_07)
	{
		if (Stream_GetRemainingLength(s) < 2)
			return FALSE;

		Stream_Read_UINT16(s, multi_draw_nine_grid->cbData);
		return update_read_delta_rects(s, multi_draw_nine_grid->rectangles,
		                               multi_draw_nine_grid->nDeltaEntries);
	}

	return TRUE;
}