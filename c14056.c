static ssize_t _hostsock_recvfrom(
    oe_fd_t* sock_,
    void* buf,
    size_t count,
    int flags,
    const struct oe_sockaddr* src_addr,
    oe_socklen_t* addrlen)
{
    ssize_t ret = -1;
    sock_t* sock = _cast_sock(sock_);
    oe_socklen_t addrlen_in = 0;

    oe_errno = 0;

    if (!sock || (count && !buf))
        OE_RAISE_ERRNO(OE_EINVAL);

    if (addrlen)
        addrlen_in = *addrlen;

    if (oe_syscall_recvfrom_ocall(
            &ret,
            sock->host_fd,
            buf,
            count,
            flags,
            (struct oe_sockaddr*)src_addr,
            addrlen_in,
            addrlen) != OE_OK)
    {
        OE_RAISE_ERRNO(OE_EINVAL);
    }

done:
    return ret;
}