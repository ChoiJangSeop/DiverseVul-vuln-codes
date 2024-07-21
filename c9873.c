static void do_cmd(ESPState *s)
{
    uint8_t busid = fifo8_pop(&s->cmdfifo);
    uint32_t n;

    s->cmdfifo_cdb_offset--;

    /* Ignore extended messages for now */
    if (s->cmdfifo_cdb_offset) {
        fifo8_pop_buf(&s->cmdfifo, s->cmdfifo_cdb_offset, &n);
        s->cmdfifo_cdb_offset = 0;
    }

    do_busid_cmd(s, busid);
}