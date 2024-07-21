static uint8_t esp_fifo_pop(ESPState *s)
{
    if (fifo8_is_empty(&s->fifo)) {
        return 0;
    }

    return fifo8_pop(&s->fifo);
}