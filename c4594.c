void qmp_chardev_remove(const char *id, Error **errp)
{
    CharDriverState *chr;

    chr = qemu_chr_find(id);
    if (chr == NULL) {
        error_setg(errp, "Chardev '%s' not found", id);
        return;
    }
    if (chr->chr_can_read || chr->chr_read ||
        chr->chr_event || chr->handler_opaque) {
        error_setg(errp, "Chardev '%s' is busy", id);
        return;
    }
    if (chr->replay) {
        error_setg(errp,
            "Chardev '%s' cannot be unplugged in record/replay mode", id);
        return;
    }
    qemu_chr_delete(chr);
}