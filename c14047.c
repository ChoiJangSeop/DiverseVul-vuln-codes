int oe_iov_sync(
    const struct oe_iovec* iov,
    int iovcnt,
    const void* buf_,
    size_t buf_size)
{
    struct oe_iovec* buf = (struct oe_iovec*)buf_;
    int ret = -1;
    int i;
    size_t n;

    /* Reject invalid parameters. */
    if (iovcnt < 0 || (iovcnt > 0 && !iov))
        goto done;

    /* Synchronize the data. */
    for (i = 0, n = buf_size; i < iovcnt; i++)
    {
        if (buf[i].iov_len != iov[i].iov_len)
            goto done;

        if (buf[i].iov_len)
        {
            if (buf[i].iov_base && !iov[i].iov_base)
                goto done;

            if (!buf[i].iov_base && iov[i].iov_base)
                goto done;

            if (!buf[i].iov_base)
                continue;

            /* Fail if buffer data is exhausted. */
            if (n < buf[i].iov_len)
                goto done;

            /* Sync the base data for this element. */
            {
                /* Note: buf[i].iov_base is an offset here (not a pointer). */
                uint8_t* src = (uint8_t*)buf[i].iov_base + (uint64_t)buf;
                size_t src_size = buf[i].iov_len;
                uint8_t* dest = (uint8_t*)iov[i].iov_base;
                size_t dest_size = iov[i].iov_len;

                if (src_size != dest_size)
                    goto done;

                if (src < (uint8_t*)buf || src > (uint8_t*)buf + buf_size)
                    goto done;

                if (oe_memcpy_s(dest, dest_size, src, src_size) != OE_OK)
                    goto done;
            }
        }

        n -= buf[i].iov_len;
    }

    ret = 0;

done:

    return ret;
}