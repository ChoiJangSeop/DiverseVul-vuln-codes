static ssize_t _hostsock_sendmsg(
    oe_fd_t* sock_,
    const struct oe_msghdr* msg,
    int flags)
{
    ssize_t ret = -1;
    sock_t* sock = _cast_sock(sock_);
    void* buf = NULL;
    size_t buf_size = 0;

    oe_errno = 0;

    /* Check the parameters. */
    if (!sock || !msg || (msg->msg_iovlen && !msg->msg_iov))
        OE_RAISE_ERRNO(OE_EINVAL);

    /* Flatten the IO vector into contiguous heap memory. */
    if (oe_iov_pack(msg->msg_iov, (int)msg->msg_iovlen, &buf, &buf_size) != 0)
        OE_RAISE_ERRNO(OE_ENOMEM);

    /* Call the host. */
    if (oe_syscall_sendmsg_ocall(
            &ret,
            sock->host_fd,
            msg->msg_name,
            msg->msg_namelen,
            buf,
            msg->msg_iovlen,
            buf_size,
            msg->msg_control,
            msg->msg_controllen,
            flags) != OE_OK)
    {
        OE_RAISE_ERRNO(OE_EINVAL);
    }

done:

    if (buf)
        oe_free(buf);

    return ret;
}