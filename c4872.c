static void n_hdlc_send_frames(struct n_hdlc *n_hdlc, struct tty_struct *tty)
{
	register int actual;
	unsigned long flags;
	struct n_hdlc_buf *tbuf;

	if (debuglevel >= DEBUG_LEVEL_INFO)	
		printk("%s(%d)n_hdlc_send_frames() called\n",__FILE__,__LINE__);
 check_again:
		
 	spin_lock_irqsave(&n_hdlc->tx_buf_list.spinlock, flags);
	if (n_hdlc->tbusy) {
		n_hdlc->woke_up = 1;
 		spin_unlock_irqrestore(&n_hdlc->tx_buf_list.spinlock, flags);
		return;
	}
	n_hdlc->tbusy = 1;
	n_hdlc->woke_up = 0;
	spin_unlock_irqrestore(&n_hdlc->tx_buf_list.spinlock, flags);

	/* get current transmit buffer or get new transmit */
	/* buffer from list of pending transmit buffers */
		
	tbuf = n_hdlc->tbuf;
	if (!tbuf)
		tbuf = n_hdlc_buf_get(&n_hdlc->tx_buf_list);
		
	while (tbuf) {
		if (debuglevel >= DEBUG_LEVEL_INFO)	
			printk("%s(%d)sending frame %p, count=%d\n",
				__FILE__,__LINE__,tbuf,tbuf->count);
			
		/* Send the next block of data to device */
		set_bit(TTY_DO_WRITE_WAKEUP, &tty->flags);
		actual = tty->ops->write(tty, tbuf->buf, tbuf->count);

		/* rollback was possible and has been done */
		if (actual == -ERESTARTSYS) {
			n_hdlc->tbuf = tbuf;
			break;
		}
		/* if transmit error, throw frame away by */
		/* pretending it was accepted by driver */
		if (actual < 0)
			actual = tbuf->count;
		
		if (actual == tbuf->count) {
			if (debuglevel >= DEBUG_LEVEL_INFO)	
				printk("%s(%d)frame %p completed\n",
					__FILE__,__LINE__,tbuf);
					
			/* free current transmit buffer */
			n_hdlc_buf_put(&n_hdlc->tx_free_buf_list, tbuf);
			
			/* this tx buffer is done */
			n_hdlc->tbuf = NULL;
			
			/* wait up sleeping writers */
			wake_up_interruptible(&tty->write_wait);
	
			/* get next pending transmit buffer */
			tbuf = n_hdlc_buf_get(&n_hdlc->tx_buf_list);
		} else {
			if (debuglevel >= DEBUG_LEVEL_INFO)	
				printk("%s(%d)frame %p pending\n",
					__FILE__,__LINE__,tbuf);
					
			/* buffer not accepted by driver */
			/* set this buffer as pending buffer */
			n_hdlc->tbuf = tbuf;
			break;
		}
	}
	
	if (!tbuf)
		clear_bit(TTY_DO_WRITE_WAKEUP, &tty->flags);
	
	/* Clear the re-entry flag */
	spin_lock_irqsave(&n_hdlc->tx_buf_list.spinlock, flags);
	n_hdlc->tbusy = 0;
	spin_unlock_irqrestore(&n_hdlc->tx_buf_list.spinlock, flags); 
	
        if (n_hdlc->woke_up)
	  goto check_again;

	if (debuglevel >= DEBUG_LEVEL_INFO)	
		printk("%s(%d)n_hdlc_send_frames() exit\n",__FILE__,__LINE__);
		
}	/* end of n_hdlc_send_frames() */