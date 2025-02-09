__blockdev_direct_IO(int rw, struct kiocb *iocb, struct inode *inode,
	struct block_device *bdev, const struct iovec *iov, loff_t offset, 
	unsigned long nr_segs, get_block_t get_block, dio_iodone_t end_io,
	int dio_lock_type)
{
	int seg;
	size_t size;
	unsigned long addr;
	unsigned blkbits = inode->i_blkbits;
	unsigned bdev_blkbits = 0;
	unsigned blocksize_mask = (1 << blkbits) - 1;
	ssize_t retval = -EINVAL;
	loff_t end = offset;
	struct dio *dio;
	int release_i_mutex = 0;
	int acquire_i_mutex = 0;

	if (rw & WRITE)
		rw = WRITE_SYNC;

	if (bdev)
		bdev_blkbits = blksize_bits(bdev_hardsect_size(bdev));

	if (offset & blocksize_mask) {
		if (bdev)
			 blkbits = bdev_blkbits;
		blocksize_mask = (1 << blkbits) - 1;
		if (offset & blocksize_mask)
			goto out;
	}

	/* Check the memory alignment.  Blocks cannot straddle pages */
	for (seg = 0; seg < nr_segs; seg++) {
		addr = (unsigned long)iov[seg].iov_base;
		size = iov[seg].iov_len;
		end += size;
		if ((addr & blocksize_mask) || (size & blocksize_mask))  {
			if (bdev)
				 blkbits = bdev_blkbits;
			blocksize_mask = (1 << blkbits) - 1;
			if ((addr & blocksize_mask) || (size & blocksize_mask))  
				goto out;
		}
	}

	dio = kmalloc(sizeof(*dio), GFP_KERNEL);
	retval = -ENOMEM;
	if (!dio)
		goto out;

	/*
	 * For block device access DIO_NO_LOCKING is used,
	 *	neither readers nor writers do any locking at all
	 * For regular files using DIO_LOCKING,
	 *	readers need to grab i_mutex and i_alloc_sem
	 *	writers need to grab i_alloc_sem only (i_mutex is already held)
	 * For regular files using DIO_OWN_LOCKING,
	 *	neither readers nor writers take any locks here
	 */
	dio->lock_type = dio_lock_type;
	if (dio_lock_type != DIO_NO_LOCKING) {
		/* watch out for a 0 len io from a tricksy fs */
		if (rw == READ && end > offset) {
			struct address_space *mapping;

			mapping = iocb->ki_filp->f_mapping;
			if (dio_lock_type != DIO_OWN_LOCKING) {
				mutex_lock(&inode->i_mutex);
				release_i_mutex = 1;
			}

			retval = filemap_write_and_wait_range(mapping, offset,
							      end - 1);
			if (retval) {
				kfree(dio);
				goto out;
			}

			if (dio_lock_type == DIO_OWN_LOCKING) {
				mutex_unlock(&inode->i_mutex);
				acquire_i_mutex = 1;
			}
		}

		if (dio_lock_type == DIO_LOCKING)
			/* lockdep: not the owner will release it */
			down_read_non_owner(&inode->i_alloc_sem);
	}

	/*
	 * For file extending writes updating i_size before data
	 * writeouts complete can expose uninitialized blocks. So
	 * even for AIO, we need to wait for i/o to complete before
	 * returning in this case.
	 */
	dio->is_async = !is_sync_kiocb(iocb) && !((rw & WRITE) &&
		(end > i_size_read(inode)));

	retval = direct_io_worker(rw, iocb, inode, iov, offset,
				nr_segs, blkbits, get_block, end_io, dio);

	if (rw == READ && dio_lock_type == DIO_LOCKING)
		release_i_mutex = 0;

out:
	if (release_i_mutex)
		mutex_unlock(&inode->i_mutex);
	else if (acquire_i_mutex)
		mutex_lock(&inode->i_mutex);
	return retval;
}