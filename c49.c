shmem_alloc_page(gfp_t gfp,struct shmem_inode_info *info, unsigned long idx)
{
	return alloc_page(gfp | __GFP_ZERO);
}