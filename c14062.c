static ssize_t _hostsock_readv(
    oe_fd_t* desc,
    const struct oe_iovec* iov,
    int iovcnt)
{
    ssize_t ret = -1;
    sock_t* sock = _cast_sock(desc);
    void* buf = NULL;
    size_t buf_size = 0;

    if (!sock || (!iov && iovcnt) || iovcnt < 0 || iovcnt > OE_IOV_MAX)
        OE_RAISE_ERRNO(OE_EINVAL);

    /* Flatten the IO vector into contiguous heap memory. */
    if (oe_iov_pack(iov, iovcnt, &buf, &buf_size) != 0)
        OE_RAISE_ERRNO(OE_ENOMEM);

    /* Call the host. */
    if (oe_syscall_recvv_ocall(&ret, sock->host_fd, buf, iovcnt, buf_size) !=
        OE_OK)
    {
        OE_RAISE_ERRNO(OE_EINVAL);
    }

    /* Synchronize data read with IO vector. */
    if (ret > 0)
    {
        if (oe_iov_sync(iov, iovcnt, buf, buf_size) != 0)
            OE_RAISE_ERRNO(OE_EINVAL);
    }

done:

    if (buf)
        oe_free(buf);

    return ret;
}