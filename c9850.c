dp_packet_set_l2_pad_size(struct dp_packet *b, uint8_t pad_size)
{
    ovs_assert(pad_size <= dp_packet_size(b));
    b->l2_pad_size = pad_size;
}