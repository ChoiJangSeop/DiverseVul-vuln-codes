static int pin_rcv_pages(struct hfi1_filedata *fd, struct tid_user_buf *tidbuf)
{
	int pinned;
	unsigned int npages;
	unsigned long vaddr = tidbuf->vaddr;
	struct page **pages = NULL;
	struct hfi1_devdata *dd = fd->uctxt->dd;

	/* Get the number of pages the user buffer spans */
	npages = num_user_pages(vaddr, tidbuf->length);
	if (!npages)
		return -EINVAL;

	if (npages > fd->uctxt->expected_count) {
		dd_dev_err(dd, "Expected buffer too big\n");
		return -EINVAL;
	}

	/* Allocate the array of struct page pointers needed for pinning */
	pages = kcalloc(npages, sizeof(*pages), GFP_KERNEL);
	if (!pages)
		return -ENOMEM;

	/*
	 * Pin all the pages of the user buffer. If we can't pin all the
	 * pages, accept the amount pinned so far and program only that.
	 * User space knows how to deal with partially programmed buffers.
	 */
	if (!hfi1_can_pin_pages(dd, fd->mm, fd->tid_n_pinned, npages)) {
		kfree(pages);
		return -ENOMEM;
	}

	pinned = hfi1_acquire_user_pages(fd->mm, vaddr, npages, true, pages);
	if (pinned <= 0) {
		kfree(pages);
		return pinned;
	}
	tidbuf->pages = pages;
	tidbuf->npages = npages;
	fd->tid_n_pinned += pinned;
	return pinned;
}