static ssize_t _hostfs_pwrite(
    oe_fd_t* desc,
    const void* buf,
    size_t count,
    oe_off_t offset)
{
    ssize_t ret = -1;
    file_t* file = _cast_file(desc);

    if (!file)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (oe_syscall_pwrite_ocall(&ret, file->host_fd, buf, count, offset) !=
        OE_OK)
        OE_RAISE_ERRNO(OE_EINVAL);

done:
    return ret;
}