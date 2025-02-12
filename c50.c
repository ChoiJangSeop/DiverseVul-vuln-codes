static int shmem_getpage(struct inode *inode, unsigned long idx,
			struct page **pagep, enum sgp_type sgp, int *type)
{
	struct address_space *mapping = inode->i_mapping;
	struct shmem_inode_info *info = SHMEM_I(inode);
	struct shmem_sb_info *sbinfo;
	struct page *filepage = *pagep;
	struct page *swappage;
	swp_entry_t *entry;
	swp_entry_t swap;
	int error;

	if (idx >= SHMEM_MAX_INDEX)
		return -EFBIG;

	if (type)
		*type = 0;

	/*
	 * Normally, filepage is NULL on entry, and either found
	 * uptodate immediately, or allocated and zeroed, or read
	 * in under swappage, which is then assigned to filepage.
	 * But shmem_readpage and shmem_write_begin pass in a locked
	 * filepage, which may be found not uptodate by other callers
	 * too, and may need to be copied from the swappage read in.
	 */
repeat:
	if (!filepage)
		filepage = find_lock_page(mapping, idx);
	if (filepage && PageUptodate(filepage))
		goto done;
	error = 0;
	if (sgp == SGP_QUICK)
		goto failed;

	spin_lock(&info->lock);
	shmem_recalc_inode(inode);
	entry = shmem_swp_alloc(info, idx, sgp);
	if (IS_ERR(entry)) {
		spin_unlock(&info->lock);
		error = PTR_ERR(entry);
		goto failed;
	}
	swap = *entry;

	if (swap.val) {
		/* Look it up and read it in.. */
		swappage = lookup_swap_cache(swap);
		if (!swappage) {
			shmem_swp_unmap(entry);
			/* here we actually do the io */
			if (type && !(*type & VM_FAULT_MAJOR)) {
				__count_vm_event(PGMAJFAULT);
				*type |= VM_FAULT_MAJOR;
			}
			spin_unlock(&info->lock);
			swappage = shmem_swapin(info, swap, idx);
			if (!swappage) {
				spin_lock(&info->lock);
				entry = shmem_swp_alloc(info, idx, sgp);
				if (IS_ERR(entry))
					error = PTR_ERR(entry);
				else {
					if (entry->val == swap.val)
						error = -ENOMEM;
					shmem_swp_unmap(entry);
				}
				spin_unlock(&info->lock);
				if (error)
					goto failed;
				goto repeat;
			}
			wait_on_page_locked(swappage);
			page_cache_release(swappage);
			goto repeat;
		}

		/* We have to do this with page locked to prevent races */
		if (TestSetPageLocked(swappage)) {
			shmem_swp_unmap(entry);
			spin_unlock(&info->lock);
			wait_on_page_locked(swappage);
			page_cache_release(swappage);
			goto repeat;
		}
		if (PageWriteback(swappage)) {
			shmem_swp_unmap(entry);
			spin_unlock(&info->lock);
			wait_on_page_writeback(swappage);
			unlock_page(swappage);
			page_cache_release(swappage);
			goto repeat;
		}
		if (!PageUptodate(swappage)) {
			shmem_swp_unmap(entry);
			spin_unlock(&info->lock);
			unlock_page(swappage);
			page_cache_release(swappage);
			error = -EIO;
			goto failed;
		}

		if (filepage) {
			shmem_swp_set(info, entry, 0);
			shmem_swp_unmap(entry);
			delete_from_swap_cache(swappage);
			spin_unlock(&info->lock);
			copy_highpage(filepage, swappage);
			unlock_page(swappage);
			page_cache_release(swappage);
			flush_dcache_page(filepage);
			SetPageUptodate(filepage);
			set_page_dirty(filepage);
			swap_free(swap);
		} else if (!(error = move_from_swap_cache(
				swappage, idx, mapping))) {
			info->flags |= SHMEM_PAGEIN;
			shmem_swp_set(info, entry, 0);
			shmem_swp_unmap(entry);
			spin_unlock(&info->lock);
			filepage = swappage;
			swap_free(swap);
		} else {
			shmem_swp_unmap(entry);
			spin_unlock(&info->lock);
			unlock_page(swappage);
			page_cache_release(swappage);
			if (error == -ENOMEM) {
				/* let kswapd refresh zone for GFP_ATOMICs */
				congestion_wait(WRITE, HZ/50);
			}
			goto repeat;
		}
	} else if (sgp == SGP_READ && !filepage) {
		shmem_swp_unmap(entry);
		filepage = find_get_page(mapping, idx);
		if (filepage &&
		    (!PageUptodate(filepage) || TestSetPageLocked(filepage))) {
			spin_unlock(&info->lock);
			wait_on_page_locked(filepage);
			page_cache_release(filepage);
			filepage = NULL;
			goto repeat;
		}
		spin_unlock(&info->lock);
	} else {
		shmem_swp_unmap(entry);
		sbinfo = SHMEM_SB(inode->i_sb);
		if (sbinfo->max_blocks) {
			spin_lock(&sbinfo->stat_lock);
			if (sbinfo->free_blocks == 0 ||
			    shmem_acct_block(info->flags)) {
				spin_unlock(&sbinfo->stat_lock);
				spin_unlock(&info->lock);
				error = -ENOSPC;
				goto failed;
			}
			sbinfo->free_blocks--;
			inode->i_blocks += BLOCKS_PER_PAGE;
			spin_unlock(&sbinfo->stat_lock);
		} else if (shmem_acct_block(info->flags)) {
			spin_unlock(&info->lock);
			error = -ENOSPC;
			goto failed;
		}

		if (!filepage) {
			spin_unlock(&info->lock);
			filepage = shmem_alloc_page(mapping_gfp_mask(mapping),
						    info,
						    idx);
			if (!filepage) {
				shmem_unacct_blocks(info->flags, 1);
				shmem_free_blocks(inode, 1);
				error = -ENOMEM;
				goto failed;
			}

			spin_lock(&info->lock);
			entry = shmem_swp_alloc(info, idx, sgp);
			if (IS_ERR(entry))
				error = PTR_ERR(entry);
			else {
				swap = *entry;
				shmem_swp_unmap(entry);
			}
			if (error || swap.val || 0 != add_to_page_cache_lru(
					filepage, mapping, idx, GFP_ATOMIC)) {
				spin_unlock(&info->lock);
				page_cache_release(filepage);
				shmem_unacct_blocks(info->flags, 1);
				shmem_free_blocks(inode, 1);
				filepage = NULL;
				if (error)
					goto failed;
				goto repeat;
			}
			info->flags |= SHMEM_PAGEIN;
		}

		info->alloced++;
		spin_unlock(&info->lock);
		flush_dcache_page(filepage);
		SetPageUptodate(filepage);
	}
done:
	if (*pagep != filepage) {
		*pagep = filepage;
		if (sgp != SGP_FAULT)
			unlock_page(filepage);

	}
	return 0;

failed:
	if (*pagep != filepage) {
		unlock_page(filepage);
		page_cache_release(filepage);
	}
	return error;
}