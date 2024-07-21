static void esp_fifo_push(ESPState *s, uint8_t val)
{
    if (fifo8_num_used(&s->fifo) == ESP_FIFO_SZ) {
        trace_esp_error_fifo_overrun();
        return;
    }

    fifo8_push(&s->fifo, val);
}