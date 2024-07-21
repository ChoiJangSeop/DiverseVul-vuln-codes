static UINT gdi_SurfaceToSurface(RdpgfxClientContext* context,
                                 const RDPGFX_SURFACE_TO_SURFACE_PDU* surfaceToSurface)
{
	UINT status = ERROR_INTERNAL_ERROR;
	UINT16 index;
	BOOL sameSurface;
	UINT32 nWidth, nHeight;
	const RECTANGLE_16* rectSrc;
	RDPGFX_POINT16* destPt;
	RECTANGLE_16 invalidRect;
	gdiGfxSurface* surfaceSrc;
	gdiGfxSurface* surfaceDst;
	rdpGdi* gdi = (rdpGdi*)context->custom;
	EnterCriticalSection(&context->mux);
	rectSrc = &(surfaceToSurface->rectSrc);
	surfaceSrc = (gdiGfxSurface*)context->GetSurfaceData(context, surfaceToSurface->surfaceIdSrc);
	sameSurface =
	    (surfaceToSurface->surfaceIdSrc == surfaceToSurface->surfaceIdDest) ? TRUE : FALSE;

	if (!sameSurface)
		surfaceDst =
		    (gdiGfxSurface*)context->GetSurfaceData(context, surfaceToSurface->surfaceIdDest);
	else
		surfaceDst = surfaceSrc;

	if (!surfaceSrc || !surfaceDst)
		goto fail;

	nWidth = rectSrc->right - rectSrc->left;
	nHeight = rectSrc->bottom - rectSrc->top;

	for (index = 0; index < surfaceToSurface->destPtsCount; index++)
	{
		destPt = &surfaceToSurface->destPts[index];

		if (!freerdp_image_copy(surfaceDst->data, surfaceDst->format, surfaceDst->scanline,
		                        destPt->x, destPt->y, nWidth, nHeight, surfaceSrc->data,
		                        surfaceSrc->format, surfaceSrc->scanline, rectSrc->left,
		                        rectSrc->top, NULL, FREERDP_FLIP_NONE))
			goto fail;

		invalidRect.left = destPt->x;
		invalidRect.top = destPt->y;
		invalidRect.right = destPt->x + rectSrc->right;
		invalidRect.bottom = destPt->y + rectSrc->bottom;
		region16_union_rect(&surfaceDst->invalidRegion, &surfaceDst->invalidRegion, &invalidRect);
		status = IFCALLRESULT(CHANNEL_RC_OK, context->UpdateSurfaceArea, context,
		                      surfaceDst->surfaceId, 1, &invalidRect);

		if (status != CHANNEL_RC_OK)
			goto fail;
	}

	LeaveCriticalSection(&context->mux);

	if (!gdi->inGfxFrame)
	{
		status = CHANNEL_RC_NOT_INITIALIZED;
		IFCALLRET(context->UpdateSurfaces, status, context);
	}

	return status;
fail:
	LeaveCriticalSection(&context->mux);
	return status;
}