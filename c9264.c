static UINT gdi_CacheToSurface(RdpgfxClientContext* context,
                               const RDPGFX_CACHE_TO_SURFACE_PDU* cacheToSurface)
{
	UINT status = ERROR_INTERNAL_ERROR;
	UINT16 index;
	RDPGFX_POINT16* destPt;
	gdiGfxSurface* surface;
	gdiGfxCacheEntry* cacheEntry;
	RECTANGLE_16 invalidRect;
	rdpGdi* gdi = (rdpGdi*)context->custom;
	EnterCriticalSection(&context->mux);
	surface = (gdiGfxSurface*)context->GetSurfaceData(context, cacheToSurface->surfaceId);
	cacheEntry = (gdiGfxCacheEntry*)context->GetCacheSlotData(context, cacheToSurface->cacheSlot);

	if (!surface || !cacheEntry)
		goto fail;

	for (index = 0; index < cacheToSurface->destPtsCount; index++)
	{
		destPt = &cacheToSurface->destPts[index];

		if (!freerdp_image_copy(surface->data, surface->format, surface->scanline, destPt->x,
		                        destPt->y, cacheEntry->width, cacheEntry->height, cacheEntry->data,
		                        cacheEntry->format, cacheEntry->scanline, 0, 0, NULL,
		                        FREERDP_FLIP_NONE))
			goto fail;

		invalidRect.left = destPt->x;
		invalidRect.top = destPt->y;
		invalidRect.right = destPt->x + cacheEntry->width;
		invalidRect.bottom = destPt->y + cacheEntry->height;
		region16_union_rect(&surface->invalidRegion, &surface->invalidRegion, &invalidRect);
		status = IFCALLRESULT(CHANNEL_RC_OK, context->UpdateSurfaceArea, context,
		                      surface->surfaceId, 1, &invalidRect);

		if (status != CHANNEL_RC_OK)
			goto fail;
	}

	LeaveCriticalSection(&context->mux);

	if (!gdi->inGfxFrame)
	{
		status = CHANNEL_RC_NOT_INITIALIZED;
		IFCALLRET(context->UpdateSurfaces, status, context);
	}
	else
		status = CHANNEL_RC_OK;

	return status;
fail:
	LeaveCriticalSection(&context->mux);
	return status;
}