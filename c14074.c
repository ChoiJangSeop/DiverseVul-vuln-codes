ssize_t oe_recvfrom(
    int sockfd,
    void* buf,
    size_t len,
    int flags,
    const struct oe_sockaddr* src_addr,
    oe_socklen_t* addrlen)
{
    ssize_t ret = -1;
    oe_fd_t* sock;

    if (!(sock = oe_fdtable_get(sockfd, OE_FD_TYPE_SOCKET)))
        OE_RAISE_ERRNO(oe_errno);

    ret = sock->ops.socket.recvfrom(sock, buf, len, flags, src_addr, addrlen);

done:
    return ret;
}