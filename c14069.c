static ssize_t _hostfs_read(oe_fd_t* desc, void* buf, size_t count)
{
    ssize_t ret = -1;
    file_t* file = _cast_file(desc);

    if (!file)
        OE_RAISE_ERRNO(OE_EINVAL);

    /* Call the host to perform the read(). */
    if (oe_syscall_read_ocall(&ret, file->host_fd, buf, count) != OE_OK)
        OE_RAISE_ERRNO(OE_EINVAL);

done:
    return ret;
}