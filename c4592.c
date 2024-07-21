void qemu_chr_be_write_impl(CharDriverState *s, uint8_t *buf, int len)
{
    if (s->chr_read) {
        s->chr_read(s->handler_opaque, buf, len);
    }
}