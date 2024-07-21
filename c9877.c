static void do_cmd(ESPState *s)
{
    uint8_t busid = fifo8_pop(&s->cmdfifo);

    s->cmdfifo_cdb_offset--;

    /* Ignore extended messages for now */
    if (s->cmdfifo_cdb_offset) {
        esp_fifo_pop_buf(&s->cmdfifo, NULL, s->cmdfifo_cdb_offset);
        s->cmdfifo_cdb_offset = 0;
    }

    do_busid_cmd(s, busid);
}