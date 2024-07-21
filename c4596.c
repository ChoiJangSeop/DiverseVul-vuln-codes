bool qemu_chr_fe_init(CharBackend *b, CharDriverState *s, Error **errp)
{
    int tag = 0;

    if (s->is_mux) {
        tag = mux_chr_new_handler_tag(s, errp);
        if (tag < 0) {
            return false;
        }
    }

    b->tag = tag;
    b->chr = s;

    return true;
}