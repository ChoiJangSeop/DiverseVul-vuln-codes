static UINT gdi_SurfaceToCache(RdpgfxClientContext* context,
                               const RDPGFX_SURFACE_TO_CACHE_PDU* surfaceToCache)
{
	const RECTANGLE_16* rect;
	gdiGfxSurface* surface;
	gdiGfxCacheEntry* cacheEntry;
	UINT rc = ERROR_INTERNAL_ERROR;
	EnterCriticalSection(&context->mux);
	rect = &(surfaceToCache->rectSrc);
	surface = (gdiGfxSurface*)context->GetSurfaceData(context, surfaceToCache->surfaceId);

	if (!surface)
		goto fail;

	cacheEntry = (gdiGfxCacheEntry*)calloc(1, sizeof(gdiGfxCacheEntry));

	if (!cacheEntry)
		goto fail;

	cacheEntry->width = (UINT32)(rect->right - rect->left);
	cacheEntry->height = (UINT32)(rect->bottom - rect->top);
	cacheEntry->format = surface->format;
	cacheEntry->scanline = gfx_align_scanline(cacheEntry->width * 4, 16);
	cacheEntry->data = (BYTE*)calloc(cacheEntry->height, cacheEntry->scanline);

	if (!cacheEntry->data)
	{
		free(cacheEntry);
		goto fail;
	}

	if (!freerdp_image_copy(cacheEntry->data, cacheEntry->format, cacheEntry->scanline, 0, 0,
	                        cacheEntry->width, cacheEntry->height, surface->data, surface->format,
	                        surface->scanline, rect->left, rect->top, NULL, FREERDP_FLIP_NONE))
	{
		free(cacheEntry->data);
		free(cacheEntry);
		goto fail;
	}

	rc = context->SetCacheSlotData(context, surfaceToCache->cacheSlot, (void*)cacheEntry);
fail:
	LeaveCriticalSection(&context->mux);
	return rc;
}