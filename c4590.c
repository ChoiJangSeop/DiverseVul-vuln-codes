static void mux_chr_close(struct CharDriverState *chr)
{
    MuxDriver *d = chr->opaque;

    qemu_chr_fe_deinit(&d->chr);
    g_free(d);
}