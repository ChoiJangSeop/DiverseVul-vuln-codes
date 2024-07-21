static int pin_sdma_pages(struct user_sdma_request *req,
			  struct user_sdma_iovec *iovec,
			  struct sdma_mmu_node *node,
			  int npages)
{
	int pinned, cleared;
	struct page **pages;
	struct hfi1_user_sdma_pkt_q *pq = req->pq;

	pages = kcalloc(npages, sizeof(*pages), GFP_KERNEL);
	if (!pages)
		return -ENOMEM;
	memcpy(pages, node->pages, node->npages * sizeof(*pages));

	npages -= node->npages;
retry:
	if (!hfi1_can_pin_pages(pq->dd, pq->mm,
				atomic_read(&pq->n_locked), npages)) {
		cleared = sdma_cache_evict(pq, npages);
		if (cleared >= npages)
			goto retry;
	}
	pinned = hfi1_acquire_user_pages(pq->mm,
					 ((unsigned long)iovec->iov.iov_base +
					 (node->npages * PAGE_SIZE)), npages, 0,
					 pages + node->npages);
	if (pinned < 0) {
		kfree(pages);
		return pinned;
	}
	if (pinned != npages) {
		unpin_vector_pages(pq->mm, pages, node->npages, pinned);
		return -EFAULT;
	}
	kfree(node->pages);
	node->rb.len = iovec->iov.iov_len;
	node->pages = pages;
	atomic_add(pinned, &pq->n_locked);
	return pinned;
}