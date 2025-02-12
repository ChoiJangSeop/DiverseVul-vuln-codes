static BOOL update_recv_secondary_order(rdpUpdate* update, wStream* s, BYTE flags)
{
	BOOL rc = FALSE;
	BYTE* next;
	BYTE orderType;
	UINT16 extraFlags;
	UINT16 orderLength;
	rdpContext* context = update->context;
	rdpSettings* settings = context->settings;
	rdpSecondaryUpdate* secondary = update->secondary;
	const char* name;

	if (Stream_GetRemainingLength(s) < 5)
	{
		WLog_Print(update->log, WLOG_ERROR, "Stream_GetRemainingLength(s) < 5");
		return FALSE;
	}

	Stream_Read_UINT16(s, orderLength); /* orderLength (2 bytes) */
	Stream_Read_UINT16(s, extraFlags);  /* extraFlags (2 bytes) */
	Stream_Read_UINT8(s, orderType);    /* orderType (1 byte) */
	next = Stream_Pointer(s) + ((INT16)orderLength) + 7;
	name = secondary_order_string(orderType);
	WLog_Print(update->log, WLOG_DEBUG, "Secondary Drawing Order %s", name);

	if (!check_secondary_order_supported(update->log, settings, orderType, name))
		return FALSE;

	switch (orderType)
	{
		case ORDER_TYPE_BITMAP_UNCOMPRESSED:
		case ORDER_TYPE_CACHE_BITMAP_COMPRESSED:
		{
			const BOOL compressed = (orderType == ORDER_TYPE_CACHE_BITMAP_COMPRESSED);
			CACHE_BITMAP_ORDER* order =
			    update_read_cache_bitmap_order(update, s, compressed, extraFlags);

			if (order)
			{
				rc = IFCALLRESULT(FALSE, secondary->CacheBitmap, context, order);
				free_cache_bitmap_order(context, order);
			}
		}
		break;

		case ORDER_TYPE_BITMAP_UNCOMPRESSED_V2:
		case ORDER_TYPE_BITMAP_COMPRESSED_V2:
		{
			const BOOL compressed = (orderType == ORDER_TYPE_BITMAP_COMPRESSED_V2);
			CACHE_BITMAP_V2_ORDER* order =
			    update_read_cache_bitmap_v2_order(update, s, compressed, extraFlags);

			if (order)
			{
				rc = IFCALLRESULT(FALSE, secondary->CacheBitmapV2, context, order);
				free_cache_bitmap_v2_order(context, order);
			}
		}
		break;

		case ORDER_TYPE_BITMAP_COMPRESSED_V3:
		{
			CACHE_BITMAP_V3_ORDER* order = update_read_cache_bitmap_v3_order(update, s, extraFlags);

			if (order)
			{
				rc = IFCALLRESULT(FALSE, secondary->CacheBitmapV3, context, order);
				free_cache_bitmap_v3_order(context, order);
			}
		}
		break;

		case ORDER_TYPE_CACHE_COLOR_TABLE:
		{
			CACHE_COLOR_TABLE_ORDER* order =
			    update_read_cache_color_table_order(update, s, extraFlags);

			if (order)
			{
				rc = IFCALLRESULT(FALSE, secondary->CacheColorTable, context, order);
				free_cache_color_table_order(context, order);
			}
		}
		break;

		case ORDER_TYPE_CACHE_GLYPH:
		{
			switch (settings->GlyphSupportLevel)
			{
				case GLYPH_SUPPORT_PARTIAL:
				case GLYPH_SUPPORT_FULL:
				{
					CACHE_GLYPH_ORDER* order = update_read_cache_glyph_order(update, s, extraFlags);

					if (order)
					{
						rc = IFCALLRESULT(FALSE, secondary->CacheGlyph, context, order);
						free_cache_glyph_order(context, order);
					}
				}
				break;

				case GLYPH_SUPPORT_ENCODE:
				{
					CACHE_GLYPH_V2_ORDER* order =
					    update_read_cache_glyph_v2_order(update, s, extraFlags);

					if (order)
					{
						rc = IFCALLRESULT(FALSE, secondary->CacheGlyphV2, context, order);
						free_cache_glyph_v2_order(context, order);
					}
				}
				break;

				case GLYPH_SUPPORT_NONE:
				default:
					break;
			}
		}
		break;

		case ORDER_TYPE_CACHE_BRUSH:
			/* [MS-RDPEGDI] 2.2.2.2.1.2.7 Cache Brush (CACHE_BRUSH_ORDER) */
			{
				CACHE_BRUSH_ORDER* order = update_read_cache_brush_order(update, s, extraFlags);

				if (order)
				{
					rc = IFCALLRESULT(FALSE, secondary->CacheBrush, context, order);
					free_cache_brush_order(context, order);
				}
			}
			break;

		default:
			WLog_Print(update->log, WLOG_WARN, "SECONDARY ORDER %s not supported", name);
			break;
	}

	if (!rc)
	{
		WLog_Print(update->log, WLOG_ERROR, "SECONDARY ORDER %s failed", name);
	}

	Stream_SetPointer(s, next);
	return rc;
}