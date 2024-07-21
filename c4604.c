static int mux_chr_can_read(void *opaque)
{
    CharDriverState *chr = opaque;
    MuxDriver *d = chr->opaque;
    int m = d->focus;

    if ((d->prod[m] - d->cons[m]) < MUX_BUFFER_SIZE)
        return 1;
    if (d->chr_can_read[m])
        return d->chr_can_read[m](d->ext_opaque[m]);
    return 0;
}