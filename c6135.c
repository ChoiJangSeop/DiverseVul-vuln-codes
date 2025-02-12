static bool add_free_nid(struct f2fs_sb_info *sbi, nid_t nid, bool build)
{
	struct f2fs_nm_info *nm_i = NM_I(sbi);
	struct free_nid *i;
	struct nat_entry *ne;
	int err;

	/* 0 nid should not be used */
	if (unlikely(nid == 0))
		return false;

	if (build) {
		/* do not add allocated nids */
		ne = __lookup_nat_cache(nm_i, nid);
		if (ne && (!get_nat_flag(ne, IS_CHECKPOINTED) ||
				nat_get_blkaddr(ne) != NULL_ADDR))
			return false;
	}

	i = f2fs_kmem_cache_alloc(free_nid_slab, GFP_NOFS);
	i->nid = nid;
	i->state = NID_NEW;

	if (radix_tree_preload(GFP_NOFS)) {
		kmem_cache_free(free_nid_slab, i);
		return true;
	}

	spin_lock(&nm_i->nid_list_lock);
	err = __insert_nid_to_list(sbi, i, FREE_NID_LIST, true);
	spin_unlock(&nm_i->nid_list_lock);
	radix_tree_preload_end();
	if (err) {
		kmem_cache_free(free_nid_slab, i);
		return true;
	}
	return true;
}