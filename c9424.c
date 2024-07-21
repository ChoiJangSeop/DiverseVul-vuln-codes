int ttm_sg_tt_init(struct ttm_dma_tt *ttm_dma, struct ttm_buffer_object *bo,
		   uint32_t page_flags)
{
	struct ttm_tt *ttm = &ttm_dma->ttm;
	int ret;

	ttm_tt_init_fields(ttm, bo, page_flags);

	INIT_LIST_HEAD(&ttm_dma->pages_list);
	if (page_flags & TTM_PAGE_FLAG_SG)
		ret = ttm_sg_tt_alloc_page_directory(ttm_dma);
	else
		ret = ttm_dma_tt_alloc_page_directory(ttm_dma);
	if (ret) {
		ttm_tt_destroy(ttm);
		pr_err("Failed allocating page table\n");
		return -ENOMEM;
	}
	return 0;
}