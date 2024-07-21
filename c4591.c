static void mux_chr_update_read_handler(CharDriverState *chr,
                                        GMainContext *context,
                                        int tag)
{
    MuxDriver *d = chr->opaque;

    assert(tag >= 0);
    assert(tag < d->mux_cnt);

    d->ext_opaque[tag] = chr->handler_opaque;
    d->chr_can_read[tag] = chr->chr_can_read;
    d->chr_read[tag] = chr->chr_read;
    d->chr_event[tag] = chr->chr_event;
}