static void unpin_rcv_pages(struct hfi1_filedata *fd,
			    struct tid_user_buf *tidbuf,
			    struct tid_rb_node *node,
			    unsigned int idx,
			    unsigned int npages,
			    bool mapped)
{
	struct page **pages;
	struct hfi1_devdata *dd = fd->uctxt->dd;

	if (mapped) {
		pci_unmap_single(dd->pcidev, node->dma_addr,
				 node->npages * PAGE_SIZE, PCI_DMA_FROMDEVICE);
		pages = &node->pages[idx];
	} else {
		pages = &tidbuf->pages[idx];
	}
	hfi1_release_user_pages(fd->mm, pages, npages, mapped);
	fd->tid_n_pinned -= npages;
}