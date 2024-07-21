static ssize_t _epoll_write(oe_fd_t* epoll_, const void* buf, size_t count)
{
    ssize_t ret = -1;
    epoll_t* epoll = _cast_epoll(epoll_);

    oe_errno = 0;

    /* Call the host. */
    if (oe_syscall_write_ocall(&ret, epoll->host_fd, buf, count) != OE_OK)
        OE_RAISE_ERRNO(OE_EINVAL);

done:
    return ret;
}