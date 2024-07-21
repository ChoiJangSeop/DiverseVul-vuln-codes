int tty_prepare_flip_string_flags(struct tty_struct *tty,
			unsigned char **chars, char **flags, size_t size)
{
	int space = tty_buffer_request_room(tty, size);
	if (likely(space)) {
		struct tty_buffer *tb = tty->buf.tail;
		*chars = tb->char_buf_ptr + tb->used;
		*flags = tb->flag_buf_ptr + tb->used;
		tb->used += space;
	}
	return space;
}