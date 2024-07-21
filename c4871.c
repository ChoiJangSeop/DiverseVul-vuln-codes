static void n_hdlc_release(struct n_hdlc *n_hdlc)
{
	struct tty_struct *tty = n_hdlc2tty (n_hdlc);
	struct n_hdlc_buf *buf;
	
	if (debuglevel >= DEBUG_LEVEL_INFO)	
		printk("%s(%d)n_hdlc_release() called\n",__FILE__,__LINE__);
		
	/* Ensure that the n_hdlcd process is not hanging on select()/poll() */
	wake_up_interruptible (&tty->read_wait);
	wake_up_interruptible (&tty->write_wait);

	if (tty->disc_data == n_hdlc)
		tty->disc_data = NULL;	/* Break the tty->n_hdlc link */

	/* Release transmit and receive buffers */
	for(;;) {
		buf = n_hdlc_buf_get(&n_hdlc->rx_free_buf_list);
		if (buf) {
			kfree(buf);
		} else
			break;
	}
	for(;;) {
		buf = n_hdlc_buf_get(&n_hdlc->tx_free_buf_list);
		if (buf) {
			kfree(buf);
		} else
			break;
	}
	for(;;) {
		buf = n_hdlc_buf_get(&n_hdlc->rx_buf_list);
		if (buf) {
			kfree(buf);
		} else
			break;
	}
	for(;;) {
		buf = n_hdlc_buf_get(&n_hdlc->tx_buf_list);
		if (buf) {
			kfree(buf);
		} else
			break;
	}
	kfree(n_hdlc->tbuf);
	kfree(n_hdlc);
	
}	/* end of n_hdlc_release() */