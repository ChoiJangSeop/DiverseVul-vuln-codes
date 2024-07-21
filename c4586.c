void qemu_chr_fe_deinit(CharBackend *b)
{
    assert(b);

    if (b->chr) {
        qemu_chr_fe_set_handlers(b, NULL, NULL, NULL, NULL, NULL);
        b->chr->avail_connections++;
        b->chr = NULL;
    }
}