static int n_hdlc_tty_ioctl(struct tty_struct *tty, struct file *file,
			    unsigned int cmd, unsigned long arg)
{
	struct n_hdlc *n_hdlc = tty2n_hdlc (tty);
	int error = 0;
	int count;
	unsigned long flags;
	
	if (debuglevel >= DEBUG_LEVEL_INFO)	
		printk("%s(%d)n_hdlc_tty_ioctl() called %d\n",
			__FILE__,__LINE__,cmd);
		
	/* Verify the status of the device */
	if (!n_hdlc || n_hdlc->magic != HDLC_MAGIC)
		return -EBADF;

	switch (cmd) {
	case FIONREAD:
		/* report count of read data available */
		/* in next available frame (if any) */
		spin_lock_irqsave(&n_hdlc->rx_buf_list.spinlock,flags);
		if (n_hdlc->rx_buf_list.head)
			count = n_hdlc->rx_buf_list.head->count;
		else
			count = 0;
		spin_unlock_irqrestore(&n_hdlc->rx_buf_list.spinlock,flags);
		error = put_user(count, (int __user *)arg);
		break;

	case TIOCOUTQ:
		/* get the pending tx byte count in the driver */
		count = tty_chars_in_buffer(tty);
		/* add size of next output frame in queue */
		spin_lock_irqsave(&n_hdlc->tx_buf_list.spinlock,flags);
		if (n_hdlc->tx_buf_list.head)
			count += n_hdlc->tx_buf_list.head->count;
		spin_unlock_irqrestore(&n_hdlc->tx_buf_list.spinlock,flags);
		error = put_user(count, (int __user *)arg);
		break;

	case TCFLSH:
		switch (arg) {
		case TCIOFLUSH:
		case TCOFLUSH:
			flush_tx_queue(tty);
		}
		/* fall through to default */

	default:
		error = n_tty_ioctl_helper(tty, file, cmd, arg);
		break;
	}
	return error;
	
}	/* end of n_hdlc_tty_ioctl() */