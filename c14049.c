int oe_iov_pack(
    const struct oe_iovec* iov,
    int iovcnt,
    void** buf_out,
    size_t* buf_size_out)
{
    int ret = -1;
    struct oe_iovec* buf = NULL;
    size_t buf_size = 0;
    size_t data_size = 0;

    if (buf_out)
        *buf_out = NULL;

    if (buf_size_out)
        *buf_size_out = 0;

    /* Reject invalid parameters. */
    if (iovcnt < 0 || (iovcnt > 0 && !iov) || !buf_out || !buf_size_out)
        goto done;

    /* Handle zero-sized iovcnt up front. */
    if (iovcnt == 0)
    {
        if (iov)
        {
            if (!(buf = oe_calloc(1, sizeof(uint64_t))))
                goto done;

            buf_size = sizeof(uint64_t);
        }

        *buf_out = buf;
        *buf_size_out = buf_size;
        buf = NULL;
        ret = 0;
        goto done;
    }

    /* Calculate the total number of data bytes. */
    for (int i = 0; i < iovcnt; i++)
        data_size += iov[i].iov_len;

    /* Calculate the total size of the resulting buffer. */
    buf_size = (sizeof(struct oe_iovec) * (size_t)iovcnt) + data_size;

    /* Allocate the output buffer. */
    if (!(buf = oe_calloc(1, buf_size)))
        goto done;

    /* Initialize the array elements. */
    {
        uint8_t* p = (uint8_t*)&buf[iovcnt];
        size_t n = data_size;
        int i;

        for (i = 0; i < iovcnt; i++)
        {
            const size_t iov_len = iov[i].iov_len;
            const void* iov_base = iov[i].iov_base;

            if (iov_len)
            {
                buf[i].iov_len = iov_len;
                buf[i].iov_base = (void*)(p - (uint8_t*)buf);

                if (!iov_base)
                    goto done;

                if (oe_memcpy_s(p, n, iov_base, iov_len) != OE_OK)
                    goto done;

                p += iov_len;
                n -= iov_len;
            }
        }

        /* Fail if the data was not exhausted. */
        if (n != 0)
            goto done;
    }

    *buf_out = buf;
    *buf_size_out = buf_size;
    buf = NULL;
    ret = 0;

done:

    if (buf)
        oe_free(buf);

    return ret;
}