int tty_insert_flip_string_fixed_flag(struct tty_struct *tty,
		const unsigned char *chars, char flag, size_t size)
{
	int copied = 0;
	do {
		int goal = min_t(size_t, size - copied, TTY_BUFFER_PAGE);
		int space = tty_buffer_request_room(tty, goal);
		struct tty_buffer *tb = tty->buf.tail;
		/* If there is no space then tb may be NULL */
		if (unlikely(space == 0))
			break;
		memcpy(tb->char_buf_ptr + tb->used, chars, space);
		memset(tb->flag_buf_ptr + tb->used, flag, space);
		tb->used += space;
		copied += space;
		chars += space;
		/* There is a small chance that we need to split the data over
		   several buffers. If this is the case we must loop */
	} while (unlikely(size > copied));
	return copied;
}