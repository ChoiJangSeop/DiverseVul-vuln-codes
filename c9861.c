static void write_response(ESPState *s)
{
    uint32_t n;

    trace_esp_write_response(s->status);

    fifo8_reset(&s->fifo);
    esp_fifo_push(s, s->status);
    esp_fifo_push(s, 0);

    if (s->dma) {
        if (s->dma_memory_write) {
            s->dma_memory_write(s->dma_opaque,
                                (uint8_t *)fifo8_pop_buf(&s->fifo, 2, &n), 2);
            s->rregs[ESP_RSTAT] = STAT_TC | STAT_ST;
            s->rregs[ESP_RINTR] |= INTR_BS | INTR_FC;
            s->rregs[ESP_RSEQ] = SEQ_CD;
        } else {
            s->pdma_cb = write_response_pdma_cb;
            esp_raise_drq(s);
            return;
        }
    } else {
        s->ti_size = 2;
        s->rregs[ESP_RFLAGS] = 2;
    }
    esp_raise_irq(s);
}