nouveau_sgdma_create_ttm(struct ttm_buffer_object *bo, uint32_t page_flags)
{
	struct nouveau_drm *drm = nouveau_bdev(bo->bdev);
	struct nouveau_sgdma_be *nvbe;

	nvbe = kzalloc(sizeof(*nvbe), GFP_KERNEL);
	if (!nvbe)
		return NULL;

	if (drm->client.device.info.family < NV_DEVICE_INFO_V0_TESLA)
		nvbe->ttm.ttm.func = &nv04_sgdma_backend;
	else
		nvbe->ttm.ttm.func = &nv50_sgdma_backend;

	if (ttm_dma_tt_init(&nvbe->ttm, bo, page_flags))
		/*
		 * A failing ttm_dma_tt_init() will call ttm_tt_destroy()
		 * and thus our nouveau_sgdma_destroy() hook, so we don't need
		 * to free nvbe here.
		 */
		return NULL;
	return &nvbe->ttm.ttm;
}