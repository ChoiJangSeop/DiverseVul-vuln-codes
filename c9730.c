static int set_rcvarray_entry(struct hfi1_filedata *fd,
			      struct tid_user_buf *tbuf,
			      u32 rcventry, struct tid_group *grp,
			      u16 pageidx, unsigned int npages)
{
	int ret;
	struct hfi1_ctxtdata *uctxt = fd->uctxt;
	struct tid_rb_node *node;
	struct hfi1_devdata *dd = uctxt->dd;
	dma_addr_t phys;
	struct page **pages = tbuf->pages + pageidx;

	/*
	 * Allocate the node first so we can handle a potential
	 * failure before we've programmed anything.
	 */
	node = kzalloc(sizeof(*node) + (sizeof(struct page *) * npages),
		       GFP_KERNEL);
	if (!node)
		return -ENOMEM;

	phys = pci_map_single(dd->pcidev,
			      __va(page_to_phys(pages[0])),
			      npages * PAGE_SIZE, PCI_DMA_FROMDEVICE);
	if (dma_mapping_error(&dd->pcidev->dev, phys)) {
		dd_dev_err(dd, "Failed to DMA map Exp Rcv pages 0x%llx\n",
			   phys);
		kfree(node);
		return -EFAULT;
	}

	node->fdata = fd;
	node->phys = page_to_phys(pages[0]);
	node->npages = npages;
	node->rcventry = rcventry;
	node->dma_addr = phys;
	node->grp = grp;
	node->freed = false;
	memcpy(node->pages, pages, sizeof(struct page *) * npages);

	if (fd->use_mn) {
		ret = mmu_interval_notifier_insert(
			&node->notifier, fd->mm,
			tbuf->vaddr + (pageidx * PAGE_SIZE), npages * PAGE_SIZE,
			&tid_mn_ops);
		if (ret)
			goto out_unmap;
		/*
		 * FIXME: This is in the wrong order, the notifier should be
		 * established before the pages are pinned by pin_rcv_pages.
		 */
		mmu_interval_read_begin(&node->notifier);
	}
	fd->entry_to_rb[node->rcventry - uctxt->expected_base] = node;

	hfi1_put_tid(dd, rcventry, PT_EXPECTED, phys, ilog2(npages) + 1);
	trace_hfi1_exp_tid_reg(uctxt->ctxt, fd->subctxt, rcventry, npages,
			       node->notifier.interval_tree.start, node->phys,
			       phys);
	return 0;

out_unmap:
	hfi1_cdbg(TID, "Failed to insert RB node %u 0x%lx, 0x%lx %d",
		  node->rcventry, node->notifier.interval_tree.start,
		  node->phys, ret);
	pci_unmap_single(dd->pcidev, phys, npages * PAGE_SIZE,
			 PCI_DMA_FROMDEVICE);
	kfree(node);
	return -EFAULT;
}