void qemu_chr_fe_set_handlers(CharBackend *b,
                              IOCanReadHandler *fd_can_read,
                              IOReadHandler *fd_read,
                              IOEventHandler *fd_event,
                              void *opaque,
                              GMainContext *context)
{
    CharDriverState *s;
    int fe_open;

    s = b->chr;
    if (!s) {
        return;
    }

    if (!opaque && !fd_can_read && !fd_read && !fd_event) {
        fe_open = 0;
        remove_fd_in_watch(s);
    } else {
        fe_open = 1;
    }
    s->chr_can_read = fd_can_read;
    s->chr_read = fd_read;
    s->chr_event = fd_event;
    s->handler_opaque = opaque;
    if (s->chr_update_read_handler) {
        s->chr_update_read_handler(s, context, b->tag);
    }

    if (!s->explicit_fe_open) {
        qemu_chr_fe_set_open(b, fe_open);
    }

    if (fe_open) {
        qemu_chr_fe_take_focus(b);
        /* We're connecting to an already opened device, so let's make sure we
           also get the open event */
        if (s->be_open) {
            qemu_chr_be_generic_open(s);
        }
    }

    if (s->is_mux) {
        mux_chr_set_handlers(s, context);
    }
}