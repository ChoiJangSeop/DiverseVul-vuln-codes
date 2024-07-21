static ssize_t _epoll_read(oe_fd_t* epoll_, void* buf, size_t count)
{
    ssize_t ret = -1;
    epoll_t* file = _cast_epoll(epoll_);

    oe_errno = 0;

    if (!file)
        OE_RAISE_ERRNO(OE_EINVAL);

    /* Call the host. */
    if (oe_syscall_read_ocall(&ret, file->host_fd, buf, count) != OE_OK)
        OE_RAISE_ERRNO(OE_EINVAL);

done:
    return ret;
}