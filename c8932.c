recvfrom_blocking(void *data)
{
    struct recvfrom_arg *arg = data;
    socklen_t len0 = arg->alen;
    ssize_t ret;
    ret = recvfrom(arg->fd, RSTRING_PTR(arg->str), RSTRING_LEN(arg->str),
                   arg->flags, &arg->buf.addr, &arg->alen);
    if (ret != -1 && len0 < arg->alen)
        arg->alen = len0;
    return (VALUE)ret;
}