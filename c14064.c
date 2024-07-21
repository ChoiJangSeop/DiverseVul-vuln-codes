static ssize_t _hostfs_write(oe_fd_t* desc, const void* buf, size_t count)
{
    ssize_t ret = -1;
    file_t* file = _cast_file(desc);

    /* Check parameters. */
    if (!file || (count && !buf))
        OE_RAISE_ERRNO(OE_EINVAL);

    /* Call the host. */
    if (oe_syscall_write_ocall(&ret, file->host_fd, buf, count) != OE_OK)
        OE_RAISE_ERRNO(OE_EINVAL);

done:
    return ret;
}