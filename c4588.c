static void fd_chr_update_read_handler(CharDriverState *chr,
                                       GMainContext *context,
                                       int tag)
{
    FDCharDriver *s = chr->opaque;

    remove_fd_in_watch(chr);
    if (s->ioc_in) {
        chr->fd_in_tag = io_add_watch_poll(s->ioc_in,
                                           fd_chr_read_poll,
                                           fd_chr_read, chr,
                                           context);
    }
}