static void flush_tx_queue(struct tty_struct *tty)
{
	struct n_hdlc *n_hdlc = tty2n_hdlc(tty);
	struct n_hdlc_buf *buf;
	unsigned long flags;

	while ((buf = n_hdlc_buf_get(&n_hdlc->tx_buf_list)))
		n_hdlc_buf_put(&n_hdlc->tx_free_buf_list, buf);
 	spin_lock_irqsave(&n_hdlc->tx_buf_list.spinlock, flags);
	if (n_hdlc->tbuf) {
		n_hdlc_buf_put(&n_hdlc->tx_free_buf_list, n_hdlc->tbuf);
		n_hdlc->tbuf = NULL;
	}
	spin_unlock_irqrestore(&n_hdlc->tx_buf_list.spinlock, flags);
}