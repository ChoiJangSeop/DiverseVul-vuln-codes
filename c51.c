shmem_alloc_page(gfp_t gfp, struct shmem_inode_info *info,
		 unsigned long idx)
{
	struct vm_area_struct pvma;
	struct page *page;

	memset(&pvma, 0, sizeof(struct vm_area_struct));
	pvma.vm_policy = mpol_shared_policy_lookup(&info->policy, idx);
	pvma.vm_pgoff = idx;
	pvma.vm_end = PAGE_SIZE;
	page = alloc_page_vma(gfp | __GFP_ZERO, &pvma, 0);
	mpol_free(pvma.vm_policy);
	return page;
}