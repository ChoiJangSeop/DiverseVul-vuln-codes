static void mux_chr_read(void *opaque, const uint8_t *buf, int size)
{
    CharDriverState *chr = opaque;
    MuxDriver *d = chr->opaque;
    int m = d->focus;
    int i;

    mux_chr_accept_input (opaque);

    for(i = 0; i < size; i++)
        if (mux_proc_byte(chr, d, buf[i])) {
            if (d->prod[m] == d->cons[m] &&
                d->chr_can_read[m] &&
                d->chr_can_read[m](d->ext_opaque[m]))
                d->chr_read[m](d->ext_opaque[m], &buf[i], 1);
            else
                d->buffer[m][d->prod[m]++ & MUX_BUFFER_MASK] = buf[i];
        }
}