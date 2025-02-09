static void entropy_available(void *opaque)
{
    RndRandom *s = RNG_RANDOM(opaque);
    uint8_t buffer[s->size];
    ssize_t len;

    len = read(s->fd, buffer, s->size);
    if (len < 0 && errno == EAGAIN) {
        return;
    }
    g_assert(len != -1);

    s->receive_func(s->opaque, buffer, len);
    s->receive_func = NULL;

    qemu_set_fd_handler(s->fd, NULL, NULL, NULL);
}