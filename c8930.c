rsock_read_nonblock(VALUE sock, VALUE length, VALUE buf, VALUE ex)
{
    rb_io_t *fptr;
    long n;
    long len = NUM2LONG(length);
    VALUE str = rsock_strbuf(buf, len);
    char *ptr;

    GetOpenFile(sock, fptr);

    if (len == 0) {
	return str;
    }

    ptr = RSTRING_PTR(str);
    n = read_buffered_data(ptr, len, fptr);
    if (n <= 0) {
	n = (long)recv(fptr->fd, ptr, len, MSG_DONTWAIT);
	if (n < 0) {
	    int e = errno;
	    if ((e == EWOULDBLOCK || e == EAGAIN)) {
		if (ex == Qfalse) return sym_wait_readable;
		rb_readwrite_syserr_fail(RB_IO_WAIT_READABLE,
					 e, "read would block");
	    }
	    rb_syserr_fail_path(e, fptr->pathv);
	}
    }
    if (len != n) {
	rb_str_modify(str);
	rb_str_set_len(str, n);
	if (str != buf) {
	    rb_str_resize(str, n);
	}
    }
    if (n == 0) {
	if (ex == Qfalse) return Qnil;
	rb_eof_error();
    }

    return str;
}