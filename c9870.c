static uint8_t esp_cmdfifo_pop(ESPState *s)
{
    if (fifo8_is_empty(&s->cmdfifo)) {
        return 0;
    }

    return fifo8_pop(&s->cmdfifo);
}