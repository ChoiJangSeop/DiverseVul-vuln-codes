static ssize_t _hostsock_recv(
    oe_fd_t* sock_,
    void* buf,
    size_t count,
    int flags)
{
    ssize_t ret = -1;
    sock_t* sock = _cast_sock(sock_);

    oe_errno = 0;

    if (!sock || (count && !buf))
        OE_RAISE_ERRNO(OE_EINVAL);

    if (buf)
    {
        if (oe_memset_s(buf, count, 0, count) != OE_OK)
            OE_RAISE_ERRNO(OE_EINVAL);
    }

    if (oe_syscall_recv_ocall(&ret, sock->host_fd, buf, count, flags) != OE_OK)
        OE_RAISE_ERRNO(OE_EINVAL);

done:
    return ret;
}