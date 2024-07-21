static void mux_chr_accept_input(CharDriverState *chr)
{
    MuxDriver *d = chr->opaque;
    int m = d->focus;

    while (d->prod[m] != d->cons[m] &&
           d->chr_can_read[m] &&
           d->chr_can_read[m](d->ext_opaque[m])) {
        d->chr_read[m](d->ext_opaque[m],
                       &d->buffer[m][d->cons[m]++ & MUX_BUFFER_MASK], 1);
    }
}