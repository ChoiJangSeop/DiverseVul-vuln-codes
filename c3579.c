static void rng_random_request_entropy(RngBackend *b, size_t size,
                                        EntropyReceiveFunc *receive_entropy,
                                        void *opaque)
{
    RndRandom *s = RNG_RANDOM(b);

    if (s->receive_func) {
        s->receive_func(s->opaque, NULL, 0);
    }

    s->receive_func = receive_entropy;
    s->opaque = opaque;
    s->size = size;

    qemu_set_fd_handler(s->fd, entropy_available, NULL, s);
}