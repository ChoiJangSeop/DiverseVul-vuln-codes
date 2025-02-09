direct_io_worker(int rw, struct kiocb *iocb, struct inode *inode, 
	const struct iovec *iov, loff_t offset, unsigned long nr_segs, 
	unsigned blkbits, get_block_t get_block, dio_iodone_t end_io,
	struct dio *dio)
{
	unsigned long user_addr; 
	unsigned long flags;
	int seg;
	ssize_t ret = 0;
	ssize_t ret2;
	size_t bytes;

	dio->bio = NULL;
	dio->inode = inode;
	dio->rw = rw;
	dio->blkbits = blkbits;
	dio->blkfactor = inode->i_blkbits - blkbits;
	dio->start_zero_done = 0;
	dio->size = 0;
	dio->block_in_file = offset >> blkbits;
	dio->blocks_available = 0;
	dio->cur_page = NULL;

	dio->boundary = 0;
	dio->reap_counter = 0;
	dio->get_block = get_block;
	dio->end_io = end_io;
	dio->map_bh.b_private = NULL;
	dio->map_bh.b_state = 0;
	dio->final_block_in_bio = -1;
	dio->next_block_for_io = -1;

	dio->page_errors = 0;
	dio->io_error = 0;
	dio->result = 0;
	dio->iocb = iocb;
	dio->i_size = i_size_read(inode);

	spin_lock_init(&dio->bio_lock);
	dio->refcount = 1;
	dio->bio_list = NULL;
	dio->waiter = NULL;

	/*
	 * In case of non-aligned buffers, we may need 2 more
	 * pages since we need to zero out first and last block.
	 */
	if (unlikely(dio->blkfactor))
		dio->pages_in_io = 2;
	else
		dio->pages_in_io = 0;

	for (seg = 0; seg < nr_segs; seg++) {
		user_addr = (unsigned long)iov[seg].iov_base;
		dio->pages_in_io +=
			((user_addr+iov[seg].iov_len +PAGE_SIZE-1)/PAGE_SIZE
				- user_addr/PAGE_SIZE);
	}

	for (seg = 0; seg < nr_segs; seg++) {
		user_addr = (unsigned long)iov[seg].iov_base;
		dio->size += bytes = iov[seg].iov_len;

		/* Index into the first page of the first block */
		dio->first_block_in_page = (user_addr & ~PAGE_MASK) >> blkbits;
		dio->final_block_in_request = dio->block_in_file +
						(bytes >> blkbits);
		/* Page fetching state */
		dio->head = 0;
		dio->tail = 0;
		dio->curr_page = 0;

		dio->total_pages = 0;
		if (user_addr & (PAGE_SIZE-1)) {
			dio->total_pages++;
			bytes -= PAGE_SIZE - (user_addr & (PAGE_SIZE - 1));
		}
		dio->total_pages += (bytes + PAGE_SIZE - 1) / PAGE_SIZE;
		dio->curr_user_address = user_addr;
	
		ret = do_direct_IO(dio);

		dio->result += iov[seg].iov_len -
			((dio->final_block_in_request - dio->block_in_file) <<
					blkbits);

		if (ret) {
			dio_cleanup(dio);
			break;
		}
	} /* end iovec loop */

	if (ret == -ENOTBLK && (rw & WRITE)) {
		/*
		 * The remaining part of the request will be
		 * be handled by buffered I/O when we return
		 */
		ret = 0;
	}
	/*
	 * There may be some unwritten disk at the end of a part-written
	 * fs-block-sized block.  Go zero that now.
	 */
	dio_zero_block(dio, 1);

	if (dio->cur_page) {
		ret2 = dio_send_cur_page(dio);
		if (ret == 0)
			ret = ret2;
		page_cache_release(dio->cur_page);
		dio->cur_page = NULL;
	}
	if (dio->bio)
		dio_bio_submit(dio);

	/* All IO is now issued, send it on its way */
	blk_run_address_space(inode->i_mapping);

	/*
	 * It is possible that, we return short IO due to end of file.
	 * In that case, we need to release all the pages we got hold on.
	 */
	dio_cleanup(dio);

	/*
	 * All block lookups have been performed. For READ requests
	 * we can let i_mutex go now that its achieved its purpose
	 * of protecting us from looking up uninitialized blocks.
	 */
	if ((rw == READ) && (dio->lock_type == DIO_LOCKING))
		mutex_unlock(&dio->inode->i_mutex);

	/*
	 * The only time we want to leave bios in flight is when a successful
	 * partial aio read or full aio write have been setup.  In that case
	 * bio completion will call aio_complete.  The only time it's safe to
	 * call aio_complete is when we return -EIOCBQUEUED, so we key on that.
	 * This had *better* be the only place that raises -EIOCBQUEUED.
	 */
	BUG_ON(ret == -EIOCBQUEUED);
	if (dio->is_async && ret == 0 && dio->result &&
	    ((rw & READ) || (dio->result == dio->size)))
		ret = -EIOCBQUEUED;

	if (ret != -EIOCBQUEUED)
		dio_await_completion(dio);

	/*
	 * Sync will always be dropping the final ref and completing the
	 * operation.  AIO can if it was a broken operation described above or
	 * in fact if all the bios race to complete before we get here.  In
	 * that case dio_complete() translates the EIOCBQUEUED into the proper
	 * return code that the caller will hand to aio_complete().
	 *
	 * This is managed by the bio_lock instead of being an atomic_t so that
	 * completion paths can drop their ref and use the remaining count to
	 * decide to wake the submission path atomically.
	 */
	spin_lock_irqsave(&dio->bio_lock, flags);
	ret2 = --dio->refcount;
	spin_unlock_irqrestore(&dio->bio_lock, flags);

	if (ret2 == 0) {
		ret = dio_complete(dio, offset, ret);
		kfree(dio);
	} else
		BUG_ON(ret != -EIOCBQUEUED);

	return ret;
}