static void esp_cmdfifo_push(ESPState *s, uint8_t val)
{
    if (fifo8_num_used(&s->cmdfifo) == ESP_CMDFIFO_SZ) {
        trace_esp_error_fifo_overrun();
        return;
    }

    fifo8_push(&s->cmdfifo, val);
}