int qemu_chr_be_can_write(CharDriverState *s)
{
    if (!s->chr_can_read)
        return 0;
    return s->chr_can_read(s->handler_opaque);
}