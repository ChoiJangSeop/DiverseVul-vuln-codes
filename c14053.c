static ssize_t _consolefs_readv(
    oe_fd_t* desc,
    const struct oe_iovec* iov,
    int iovcnt)
{
    ssize_t ret = -1;
    file_t* file = _cast_file(desc);
    void* buf = NULL;
    size_t buf_size = 0;

    if (!file || !iov || iovcnt < 0 || iovcnt > OE_IOV_MAX)
        OE_RAISE_ERRNO(OE_EINVAL);

    /* Flatten the IO vector into contiguous heap memory. */
    if (oe_iov_pack(iov, iovcnt, &buf, &buf_size) != 0)
        OE_RAISE_ERRNO(OE_ENOMEM);

    /* Call the host. */
    if (oe_syscall_readv_ocall(&ret, file->host_fd, buf, iovcnt, buf_size) !=
        OE_OK)
    {
        OE_RAISE_ERRNO(OE_EINVAL);
    }

    /* Synchronize data read with IO vector. */
    if (oe_iov_sync(iov, iovcnt, buf, buf_size) != 0)
        OE_RAISE_ERRNO(OE_EINVAL);

done:

    if (buf)
        oe_free(buf);

    return ret;
}