int tty_buffer_request_room(struct tty_struct *tty, size_t size)
{
	struct tty_buffer *b, *n;
	int left;
	unsigned long flags;

	spin_lock_irqsave(&tty->buf.lock, flags);

	/* OPTIMISATION: We could keep a per tty "zero" sized buffer to
	   remove this conditional if its worth it. This would be invisible
	   to the callers */
	if ((b = tty->buf.tail) != NULL)
		left = b->size - b->used;
	else
		left = 0;

	if (left < size) {
		/* This is the slow path - looking for new buffers to use */
		if ((n = tty_buffer_find(tty, size)) != NULL) {
			if (b != NULL) {
				b->next = n;
				b->commit = b->used;
			} else
				tty->buf.head = n;
			tty->buf.tail = n;
		} else
			size = left;
	}

	spin_unlock_irqrestore(&tty->buf.lock, flags);
	return size;
}