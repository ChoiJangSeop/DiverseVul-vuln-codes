static void pty_chr_update_read_handler(CharDriverState *chr,
                                        GMainContext *context,
                                        int tag)
{
    qemu_mutex_lock(&chr->chr_write_lock);
    pty_chr_update_read_handler_locked(chr);
    qemu_mutex_unlock(&chr->chr_write_lock);
}