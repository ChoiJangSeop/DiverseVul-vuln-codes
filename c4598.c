static void tcp_chr_update_read_handler(CharDriverState *chr,
                                        GMainContext *context,
                                        int tag)
{
    TCPCharDriver *s = chr->opaque;

    if (!s->connected) {
        return;
    }

    remove_fd_in_watch(chr);
    if (s->ioc) {
        chr->fd_in_tag = io_add_watch_poll(s->ioc,
                                           tcp_chr_read_poll,
                                           tcp_chr_read, chr,
                                           context);
    }
}