int ttm_dma_tt_init(struct ttm_dma_tt *ttm_dma, struct ttm_buffer_object *bo,
		    uint32_t page_flags)
{
	struct ttm_tt *ttm = &ttm_dma->ttm;

	ttm_tt_init_fields(ttm, bo, page_flags);

	INIT_LIST_HEAD(&ttm_dma->pages_list);
	if (ttm_dma_tt_alloc_page_directory(ttm_dma)) {
		ttm_tt_destroy(ttm);
		pr_err("Failed allocating page table\n");
		return -ENOMEM;
	}
	return 0;
}