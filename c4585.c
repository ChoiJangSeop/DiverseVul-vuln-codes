void qemu_chr_be_event(CharDriverState *s, int event)
{
    /* Keep track if the char device is open */
    switch (event) {
        case CHR_EVENT_OPENED:
            s->be_open = 1;
            break;
        case CHR_EVENT_CLOSED:
            s->be_open = 0;
            break;
    }

    if (!s->chr_event)
        return;
    s->chr_event(s->handler_opaque, event);
}