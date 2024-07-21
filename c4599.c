static void udp_chr_update_read_handler(CharDriverState *chr,
                                        GMainContext *context,
                                        int tag)
{
    NetCharDriver *s = chr->opaque;

    remove_fd_in_watch(chr);
    if (s->ioc) {
        chr->fd_in_tag = io_add_watch_poll(s->ioc,
                                           udp_chr_read_poll,
                                           udp_chr_read, chr,
                                           context);
    }
}