int hfi1_user_sdma_alloc_queues(struct hfi1_ctxtdata *uctxt,
				struct hfi1_filedata *fd)
{
	int ret = -ENOMEM;
	char buf[64];
	struct hfi1_devdata *dd;
	struct hfi1_user_sdma_comp_q *cq;
	struct hfi1_user_sdma_pkt_q *pq;

	if (!uctxt || !fd)
		return -EBADF;

	if (!hfi1_sdma_comp_ring_size)
		return -EINVAL;

	dd = uctxt->dd;

	pq = kzalloc(sizeof(*pq), GFP_KERNEL);
	if (!pq)
		return -ENOMEM;
	pq->dd = dd;
	pq->ctxt = uctxt->ctxt;
	pq->subctxt = fd->subctxt;
	pq->n_max_reqs = hfi1_sdma_comp_ring_size;
	atomic_set(&pq->n_reqs, 0);
	init_waitqueue_head(&pq->wait);
	atomic_set(&pq->n_locked, 0);
	pq->mm = fd->mm;

	iowait_init(&pq->busy, 0, NULL, NULL, defer_packet_queue,
		    activate_packet_queue, NULL, NULL);
	pq->reqidx = 0;

	pq->reqs = kcalloc(hfi1_sdma_comp_ring_size,
			   sizeof(*pq->reqs),
			   GFP_KERNEL);
	if (!pq->reqs)
		goto pq_reqs_nomem;

	pq->req_in_use = kcalloc(BITS_TO_LONGS(hfi1_sdma_comp_ring_size),
				 sizeof(*pq->req_in_use),
				 GFP_KERNEL);
	if (!pq->req_in_use)
		goto pq_reqs_no_in_use;

	snprintf(buf, 64, "txreq-kmem-cache-%u-%u-%u", dd->unit, uctxt->ctxt,
		 fd->subctxt);
	pq->txreq_cache = kmem_cache_create(buf,
					    sizeof(struct user_sdma_txreq),
					    L1_CACHE_BYTES,
					    SLAB_HWCACHE_ALIGN,
					    NULL);
	if (!pq->txreq_cache) {
		dd_dev_err(dd, "[%u] Failed to allocate TxReq cache\n",
			   uctxt->ctxt);
		goto pq_txreq_nomem;
	}

	cq = kzalloc(sizeof(*cq), GFP_KERNEL);
	if (!cq)
		goto cq_nomem;

	cq->comps = vmalloc_user(PAGE_ALIGN(sizeof(*cq->comps)
				 * hfi1_sdma_comp_ring_size));
	if (!cq->comps)
		goto cq_comps_nomem;

	cq->nentries = hfi1_sdma_comp_ring_size;

	ret = hfi1_mmu_rb_register(pq, pq->mm, &sdma_rb_ops, dd->pport->hfi1_wq,
				   &pq->handler);
	if (ret) {
		dd_dev_err(dd, "Failed to register with MMU %d", ret);
		goto pq_mmu_fail;
	}

	rcu_assign_pointer(fd->pq, pq);
	fd->cq = cq;

	return 0;

pq_mmu_fail:
	vfree(cq->comps);
cq_comps_nomem:
	kfree(cq);
cq_nomem:
	kmem_cache_destroy(pq->txreq_cache);
pq_txreq_nomem:
	kfree(pq->req_in_use);
pq_reqs_no_in_use:
	kfree(pq->reqs);
pq_reqs_nomem:
	kfree(pq);

	return ret;
}