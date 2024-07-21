proc_file_read(struct file *file, char __user *buf, size_t nbytes,
	       loff_t *ppos)
{
	struct inode * inode = file->f_dentry->d_inode;
	char 	*page;
	ssize_t	retval=0;
	int	eof=0;
	ssize_t	n, count;
	char	*start;
	struct proc_dir_entry * dp;

	dp = PDE(inode);
	if (!(page = (char*) __get_free_page(GFP_KERNEL)))
		return -ENOMEM;

	while ((nbytes > 0) && !eof) {
		count = min_t(size_t, PROC_BLOCK_SIZE, nbytes);

		start = NULL;
		if (dp->get_info) {
			/* Handle old net routines */
			n = dp->get_info(page, &start, *ppos, count);
			if (n < count)
				eof = 1;
		} else if (dp->read_proc) {
			/*
			 * How to be a proc read function
			 * ------------------------------
			 * Prototype:
			 *    int f(char *buffer, char **start, off_t offset,
			 *          int count, int *peof, void *dat)
			 *
			 * Assume that the buffer is "count" bytes in size.
			 *
			 * If you know you have supplied all the data you
			 * have, set *peof.
			 *
			 * You have three ways to return data:
			 * 0) Leave *start = NULL.  (This is the default.)
			 *    Put the data of the requested offset at that
			 *    offset within the buffer.  Return the number (n)
			 *    of bytes there are from the beginning of the
			 *    buffer up to the last byte of data.  If the
			 *    number of supplied bytes (= n - offset) is 
			 *    greater than zero and you didn't signal eof
			 *    and the reader is prepared to take more data
			 *    you will be called again with the requested
			 *    offset advanced by the number of bytes 
			 *    absorbed.  This interface is useful for files
			 *    no larger than the buffer.
			 * 1) Set *start = an unsigned long value less than
			 *    the buffer address but greater than zero.
			 *    Put the data of the requested offset at the
			 *    beginning of the buffer.  Return the number of
			 *    bytes of data placed there.  If this number is
			 *    greater than zero and you didn't signal eof
			 *    and the reader is prepared to take more data
			 *    you will be called again with the requested
			 *    offset advanced by *start.  This interface is
			 *    useful when you have a large file consisting
			 *    of a series of blocks which you want to count
			 *    and return as wholes.
			 *    (Hack by Paul.Russell@rustcorp.com.au)
			 * 2) Set *start = an address within the buffer.
			 *    Put the data of the requested offset at *start.
			 *    Return the number of bytes of data placed there.
			 *    If this number is greater than zero and you
			 *    didn't signal eof and the reader is prepared to
			 *    take more data you will be called again with the
			 *    requested offset advanced by the number of bytes
			 *    absorbed.
			 */
			n = dp->read_proc(page, &start, *ppos,
					  count, &eof, dp->data);
		} else
			break;

		if (n == 0)   /* end of file */
			break;
		if (n < 0) {  /* error */
			if (retval == 0)
				retval = n;
			break;
		}

		if (start == NULL) {
			if (n > PAGE_SIZE) {
				printk(KERN_ERR
				       "proc_file_read: Apparent buffer overflow!\n");
				n = PAGE_SIZE;
			}
			n -= *ppos;
			if (n <= 0)
				break;
			if (n > count)
				n = count;
			start = page + *ppos;
		} else if (start < page) {
			if (n > PAGE_SIZE) {
				printk(KERN_ERR
				       "proc_file_read: Apparent buffer overflow!\n");
				n = PAGE_SIZE;
			}
			if (n > count) {
				/*
				 * Don't reduce n because doing so might
				 * cut off part of a data block.
				 */
				printk(KERN_WARNING
				       "proc_file_read: Read count exceeded\n");
			}
		} else /* start >= page */ {
			unsigned long startoff = (unsigned long)(start - page);
			if (n > (PAGE_SIZE - startoff)) {
				printk(KERN_ERR
				       "proc_file_read: Apparent buffer overflow!\n");
				n = PAGE_SIZE - startoff;
			}
			if (n > count)
				n = count;
		}
		
 		n -= copy_to_user(buf, start < page ? page : start, n);
		if (n == 0) {
			if (retval == 0)
				retval = -EFAULT;
			break;
		}

		*ppos += start < page ? (unsigned long)start : n;
		nbytes -= n;
		buf += n;
		retval += n;
	}
	free_page((unsigned long) page);
	return retval;
}