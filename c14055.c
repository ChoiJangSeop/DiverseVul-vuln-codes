static int _hostsock_getsockopt(
    oe_fd_t* sock_,
    int level,
    int optname,
    void* optval,
    oe_socklen_t* optlen)
{
    int ret = -1;
    sock_t* sock = _cast_sock(sock_);
    oe_socklen_t optlen_in = 0;

    oe_errno = 0;

    if (!sock)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (optlen)
        optlen_in = *optlen;

    if (oe_syscall_getsockopt_ocall(
            &ret, sock->host_fd, level, optname, optval, optlen_in, optlen) !=
        OE_OK)
    {
        OE_RAISE_ERRNO(OE_EINVAL);
    }

done:

    return ret;
}