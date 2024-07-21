static int _hostsock_getpeername(
    oe_fd_t* sock_,
    struct oe_sockaddr* addr,
    oe_socklen_t* addrlen)
{
    int ret = -1;
    sock_t* sock = _cast_sock(sock_);
    oe_socklen_t addrlen_in = 0;

    oe_errno = 0;

    if (!sock)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (addrlen)
        addrlen_in = *addrlen;

    if (oe_syscall_getpeername_ocall(
            &ret,
            sock->host_fd,
            (struct oe_sockaddr*)addr,
            addrlen_in,
            addrlen) != OE_OK)
    {
        OE_RAISE_ERRNO(OE_EINVAL);
    }

done:

    return ret;
}