static int stellaris_enet_load(QEMUFile *f, void *opaque, int version_id)
{
    stellaris_enet_state *s = (stellaris_enet_state *)opaque;
    int i;

    if (version_id != 1)
        return -EINVAL;

    s->ris = qemu_get_be32(f);
    s->im = qemu_get_be32(f);
    s->rctl = qemu_get_be32(f);
    s->tctl = qemu_get_be32(f);
    s->thr = qemu_get_be32(f);
    s->mctl = qemu_get_be32(f);
    s->mdv = qemu_get_be32(f);
    s->mtxd = qemu_get_be32(f);
    s->mrxd = qemu_get_be32(f);
    s->np = qemu_get_be32(f);
    s->tx_fifo_len = qemu_get_be32(f);
    qemu_get_buffer(f, s->tx_fifo, sizeof(s->tx_fifo));
    for (i = 0; i < 31; i++) {
        s->rx[i].len = qemu_get_be32(f);
        qemu_get_buffer(f, s->rx[i].data, sizeof(s->rx[i].data));

    }
    s->next_packet = qemu_get_be32(f);
    s->rx_fifo_offset = qemu_get_be32(f);

    return 0;
}