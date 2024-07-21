uint64_t esp_reg_read(ESPState *s, uint32_t saddr)
{
    uint32_t val;

    switch (saddr) {
    case ESP_FIFO:
        if (s->dma_memory_read && s->dma_memory_write &&
                (s->rregs[ESP_RSTAT] & STAT_PIO_MASK) == 0) {
            /* Data out.  */
            qemu_log_mask(LOG_UNIMP, "esp: PIO data read not implemented\n");
            s->rregs[ESP_FIFO] = 0;
        } else {
            s->rregs[ESP_FIFO] = esp_fifo_pop(s);
        }
        val = s->rregs[ESP_FIFO];
        break;
    case ESP_RINTR:
        /*
         * Clear sequence step, interrupt register and all status bits
         * except TC
         */
        val = s->rregs[ESP_RINTR];
        s->rregs[ESP_RINTR] = 0;
        s->rregs[ESP_RSTAT] &= ~STAT_TC;
        s->rregs[ESP_RSEQ] = SEQ_0;
        esp_lower_irq(s);
        break;
    case ESP_TCHI:
        /* Return the unique id if the value has never been written */
        if (!s->tchi_written) {
            val = s->chip_id;
        } else {
            val = s->rregs[saddr];
        }
        break;
     case ESP_RFLAGS:
        /* Bottom 5 bits indicate number of bytes in FIFO */
        val = fifo8_num_used(&s->fifo);
        break;
    default:
        val = s->rregs[saddr];
        break;
    }

    trace_esp_mem_readb(saddr, val);
    return val;
}