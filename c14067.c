static ssize_t _hostsock_send(
    oe_fd_t* sock_,
    const void* buf,
    size_t count,
    int flags)
{
    ssize_t ret = -1;
    sock_t* sock = _cast_sock(sock_);

    oe_errno = 0;

    if (!sock || (count && !buf))
        OE_RAISE_ERRNO(OE_EINVAL);

    if (oe_syscall_send_ocall(&ret, sock->host_fd, buf, count, flags) != OE_OK)
        OE_RAISE_ERRNO(OE_EINVAL);

done:
    return ret;
}