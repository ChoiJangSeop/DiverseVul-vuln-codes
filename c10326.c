static void fuse_readahead(struct readahead_control *rac)
{
	struct inode *inode = rac->mapping->host;
	struct fuse_conn *fc = get_fuse_conn(inode);
	unsigned int i, max_pages, nr_pages = 0;

	if (is_bad_inode(inode))
		return;

	max_pages = min_t(unsigned int, fc->max_pages,
			fc->max_read / PAGE_SIZE);

	for (;;) {
		struct fuse_io_args *ia;
		struct fuse_args_pages *ap;

		nr_pages = readahead_count(rac) - nr_pages;
		if (nr_pages > max_pages)
			nr_pages = max_pages;
		if (nr_pages == 0)
			break;
		ia = fuse_io_alloc(NULL, nr_pages);
		if (!ia)
			return;
		ap = &ia->ap;
		nr_pages = __readahead_batch(rac, ap->pages, nr_pages);
		for (i = 0; i < nr_pages; i++) {
			fuse_wait_on_page_writeback(inode,
						    readahead_index(rac) + i);
			ap->descs[i].length = PAGE_SIZE;
		}
		ap->num_pages = nr_pages;
		fuse_send_readpages(ia, rac->file);
	}
}