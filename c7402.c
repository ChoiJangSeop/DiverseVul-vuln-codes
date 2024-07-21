static void aio_complete_rw(struct kiocb *kiocb, long res, long res2)
{
	struct aio_kiocb *iocb = container_of(kiocb, struct aio_kiocb, rw);

	if (!list_empty_careful(&iocb->ki_list))
		aio_remove_iocb(iocb);

	if (kiocb->ki_flags & IOCB_WRITE) {
		struct inode *inode = file_inode(kiocb->ki_filp);

		/*
		 * Tell lockdep we inherited freeze protection from submission
		 * thread.
		 */
		if (S_ISREG(inode->i_mode))
			__sb_writers_acquired(inode->i_sb, SB_FREEZE_WRITE);
		file_end_write(kiocb->ki_filp);
	}

	fput(kiocb->ki_filp);
	aio_complete(iocb, res, res2);
}