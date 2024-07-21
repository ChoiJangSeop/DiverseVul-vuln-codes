static uint8_t esp_pdma_read(ESPState *s)
{
    uint8_t val;

    if (s->do_cmd) {
        val = esp_cmdfifo_pop(s);
    } else {
        val = esp_fifo_pop(s);
    }

    return val;
}