static BOOL update_recv_primary_order(rdpUpdate* update, wStream* s, BYTE flags)
{
	BOOL rc = FALSE;
	rdpContext* context = update->context;
	rdpPrimaryUpdate* primary = update->primary;
	ORDER_INFO* orderInfo = &(primary->order_info);
	rdpSettings* settings = context->settings;
	const char* orderName;

	if (flags & ORDER_TYPE_CHANGE)
	{
		if (Stream_GetRemainingLength(s) < 1)
		{
			WLog_Print(update->log, WLOG_ERROR, "Stream_GetRemainingLength(s) < 1");
			return FALSE;
		}

		Stream_Read_UINT8(s, orderInfo->orderType); /* orderType (1 byte) */
	}

	orderName = primary_order_string(orderInfo->orderType);

	if (!check_primary_order_supported(update->log, settings, orderInfo->orderType, orderName))
		return FALSE;

	if (!update_read_field_flags(s, &(orderInfo->fieldFlags), flags,
	                             PRIMARY_DRAWING_ORDER_FIELD_BYTES[orderInfo->orderType]))
	{
		WLog_Print(update->log, WLOG_ERROR, "update_read_field_flags() failed");
		return FALSE;
	}

	if (flags & ORDER_BOUNDS)
	{
		if (!(flags & ORDER_ZERO_BOUNDS_DELTAS))
		{
			if (!update_read_bounds(s, &orderInfo->bounds))
			{
				WLog_Print(update->log, WLOG_ERROR, "update_read_bounds() failed");
				return FALSE;
			}
		}

		rc = IFCALLRESULT(FALSE, update->SetBounds, context, &orderInfo->bounds);

		if (!rc)
			return FALSE;
	}

	orderInfo->deltaCoordinates = (flags & ORDER_DELTA_COORDINATES) ? TRUE : FALSE;

	if (!read_primary_order(update->log, orderName, s, orderInfo, primary))
		return FALSE;

	switch (orderInfo->orderType)
	{
		case ORDER_TYPE_DSTBLT:
		{
			WLog_Print(update->log, WLOG_DEBUG, "Primary Drawing Order %s rop=%s [0x%08" PRIx32 "]",
			           orderName, gdi_rop3_code_string(primary->dstblt.bRop),
			           gdi_rop3_code(primary->dstblt.bRop));
			rc = IFCALLRESULT(FALSE, primary->DstBlt, context, &primary->dstblt);
		}
		break;

		case ORDER_TYPE_PATBLT:
		{
			WLog_Print(update->log, WLOG_DEBUG, "Primary Drawing Order %s rop=%s [0x%08" PRIx32 "]",
			           orderName, gdi_rop3_code_string(primary->patblt.bRop),
			           gdi_rop3_code(primary->patblt.bRop));
			rc = IFCALLRESULT(FALSE, primary->PatBlt, context, &primary->patblt);
		}
		break;

		case ORDER_TYPE_SCRBLT:
		{
			WLog_Print(update->log, WLOG_DEBUG, "Primary Drawing Order %s rop=%s [0x%08" PRIx32 "]",
			           orderName, gdi_rop3_code_string(primary->scrblt.bRop),
			           gdi_rop3_code(primary->scrblt.bRop));
			rc = IFCALLRESULT(FALSE, primary->ScrBlt, context, &primary->scrblt);
		}
		break;

		case ORDER_TYPE_OPAQUE_RECT:
		{
			WLog_Print(update->log, WLOG_DEBUG, "Primary Drawing Order %s", orderName);
			rc = IFCALLRESULT(FALSE, primary->OpaqueRect, context, &primary->opaque_rect);
		}
		break;

		case ORDER_TYPE_DRAW_NINE_GRID:
		{
			WLog_Print(update->log, WLOG_DEBUG, "Primary Drawing Order %s", orderName);
			rc = IFCALLRESULT(FALSE, primary->DrawNineGrid, context, &primary->draw_nine_grid);
		}
		break;

		case ORDER_TYPE_MULTI_DSTBLT:
		{
			WLog_Print(update->log, WLOG_DEBUG, "Primary Drawing Order %s rop=%s [0x%08" PRIx32 "]",
			           orderName, gdi_rop3_code_string(primary->multi_dstblt.bRop),
			           gdi_rop3_code(primary->multi_dstblt.bRop));
			rc = IFCALLRESULT(FALSE, primary->MultiDstBlt, context, &primary->multi_dstblt);
		}
		break;

		case ORDER_TYPE_MULTI_PATBLT:
		{
			WLog_Print(update->log, WLOG_DEBUG, "Primary Drawing Order %s rop=%s [0x%08" PRIx32 "]",
			           orderName, gdi_rop3_code_string(primary->multi_patblt.bRop),
			           gdi_rop3_code(primary->multi_patblt.bRop));
			rc = IFCALLRESULT(FALSE, primary->MultiPatBlt, context, &primary->multi_patblt);
		}
		break;

		case ORDER_TYPE_MULTI_SCRBLT:
		{
			WLog_Print(update->log, WLOG_DEBUG, "Primary Drawing Order %s rop=%s [0x%08" PRIx32 "]",
			           orderName, gdi_rop3_code_string(primary->multi_scrblt.bRop),
			           gdi_rop3_code(primary->multi_scrblt.bRop));
			rc = IFCALLRESULT(FALSE, primary->MultiScrBlt, context, &primary->multi_scrblt);
		}
		break;

		case ORDER_TYPE_MULTI_OPAQUE_RECT:
		{
			WLog_Print(update->log, WLOG_DEBUG, "Primary Drawing Order %s", orderName);
			rc =
			    IFCALLRESULT(FALSE, primary->MultiOpaqueRect, context, &primary->multi_opaque_rect);
		}
		break;

		case ORDER_TYPE_MULTI_DRAW_NINE_GRID:
		{
			WLog_Print(update->log, WLOG_DEBUG, "Primary Drawing Order %s", orderName);
			rc = IFCALLRESULT(FALSE, primary->MultiDrawNineGrid, context,
			                  &primary->multi_draw_nine_grid);
		}
		break;

		case ORDER_TYPE_LINE_TO:
		{
			WLog_Print(update->log, WLOG_DEBUG, "Primary Drawing Order %s", orderName);
			rc = IFCALLRESULT(FALSE, primary->LineTo, context, &primary->line_to);
		}
		break;

		case ORDER_TYPE_POLYLINE:
		{
			WLog_Print(update->log, WLOG_DEBUG, "Primary Drawing Order %s", orderName);
			rc = IFCALLRESULT(FALSE, primary->Polyline, context, &primary->polyline);
		}
		break;

		case ORDER_TYPE_MEMBLT:
		{
			WLog_Print(update->log, WLOG_DEBUG, "Primary Drawing Order %s rop=%s [0x%08" PRIx32 "]",
			           orderName, gdi_rop3_code_string(primary->memblt.bRop),
			           gdi_rop3_code(primary->memblt.bRop));
			rc = IFCALLRESULT(FALSE, primary->MemBlt, context, &primary->memblt);
		}
		break;

		case ORDER_TYPE_MEM3BLT:
		{
			WLog_Print(update->log, WLOG_DEBUG, "Primary Drawing Order %s rop=%s [0x%08" PRIx32 "]",
			           orderName, gdi_rop3_code_string(primary->mem3blt.bRop),
			           gdi_rop3_code(primary->mem3blt.bRop));
			rc = IFCALLRESULT(FALSE, primary->Mem3Blt, context, &primary->mem3blt);
		}
		break;

		case ORDER_TYPE_SAVE_BITMAP:
		{
			WLog_Print(update->log, WLOG_DEBUG, "Primary Drawing Order %s", orderName);
			rc = IFCALLRESULT(FALSE, primary->SaveBitmap, context, &primary->save_bitmap);
		}
		break;

		case ORDER_TYPE_GLYPH_INDEX:
		{
			WLog_Print(update->log, WLOG_DEBUG, "Primary Drawing Order %s", orderName);
			rc = IFCALLRESULT(FALSE, primary->GlyphIndex, context, &primary->glyph_index);
		}
		break;

		case ORDER_TYPE_FAST_INDEX:
		{
			WLog_Print(update->log, WLOG_DEBUG, "Primary Drawing Order %s", orderName);
			rc = IFCALLRESULT(FALSE, primary->FastIndex, context, &primary->fast_index);
		}
		break;

		case ORDER_TYPE_FAST_GLYPH:
		{
			WLog_Print(update->log, WLOG_DEBUG, "Primary Drawing Order %s", orderName);
			rc = IFCALLRESULT(FALSE, primary->FastGlyph, context, &primary->fast_glyph);
		}
		break;

		case ORDER_TYPE_POLYGON_SC:
		{
			WLog_Print(update->log, WLOG_DEBUG, "Primary Drawing Order %s", orderName);
			rc = IFCALLRESULT(FALSE, primary->PolygonSC, context, &primary->polygon_sc);
		}
		break;

		case ORDER_TYPE_POLYGON_CB:
		{
			WLog_Print(update->log, WLOG_DEBUG, "Primary Drawing Order %s", orderName);
			rc = IFCALLRESULT(FALSE, primary->PolygonCB, context, &primary->polygon_cb);
		}
		break;

		case ORDER_TYPE_ELLIPSE_SC:
		{
			WLog_Print(update->log, WLOG_DEBUG, "Primary Drawing Order %s", orderName);
			rc = IFCALLRESULT(FALSE, primary->EllipseSC, context, &primary->ellipse_sc);
		}
		break;

		case ORDER_TYPE_ELLIPSE_CB:
		{
			WLog_Print(update->log, WLOG_DEBUG, "Primary Drawing Order %s", orderName);
			rc = IFCALLRESULT(FALSE, primary->EllipseCB, context, &primary->ellipse_cb);
		}
		break;

		default:
			WLog_Print(update->log, WLOG_WARN, "Primary Drawing Order %s not supported", orderName);
			break;
	}

	if (!rc)
	{
		WLog_Print(update->log, WLOG_WARN, "Primary Drawing Order %s failed", orderName);
		return FALSE;
	}

	if (flags & ORDER_BOUNDS)
	{
		rc = IFCALLRESULT(FALSE, update->SetBounds, context, NULL);
	}

	return rc;
}