int ttm_tt_init(struct ttm_tt *ttm, struct ttm_buffer_object *bo,
		uint32_t page_flags)
{
	ttm_tt_init_fields(ttm, bo, page_flags);

	if (ttm_tt_alloc_page_directory(ttm)) {
		ttm_tt_destroy(ttm);
		pr_err("Failed allocating page table\n");
		return -ENOMEM;
	}
	return 0;
}