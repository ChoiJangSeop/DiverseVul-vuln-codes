int tty_prepare_flip_string(struct tty_struct *tty, unsigned char **chars,
								size_t size)
{
	int space = tty_buffer_request_room(tty, size);
	if (likely(space)) {
		struct tty_buffer *tb = tty->buf.tail;
		*chars = tb->char_buf_ptr + tb->used;
		memset(tb->flag_buf_ptr + tb->used, TTY_NORMAL, space);
		tb->used += space;
	}
	return space;
}