static void esp_do_dma(ESPState *s)
{
    uint32_t len, cmdlen;
    int to_device = ((s->rregs[ESP_RSTAT] & 7) == STAT_DO);
    uint8_t buf[ESP_CMDFIFO_SZ];

    len = esp_get_tc(s);
    if (s->do_cmd) {
        /*
         * handle_ti_cmd() case: esp_do_dma() is called only from
         * handle_ti_cmd() with do_cmd != NULL (see the assert())
         */
        cmdlen = fifo8_num_used(&s->cmdfifo);
        trace_esp_do_dma(cmdlen, len);
        if (s->dma_memory_read) {
            s->dma_memory_read(s->dma_opaque, buf, len);
            fifo8_push_all(&s->cmdfifo, buf, len);
        } else {
            s->pdma_cb = do_dma_pdma_cb;
            esp_raise_drq(s);
            return;
        }
        trace_esp_handle_ti_cmd(cmdlen);
        s->ti_size = 0;
        if ((s->rregs[ESP_RSTAT] & 7) == STAT_CD) {
            /* No command received */
            if (s->cmdfifo_cdb_offset == fifo8_num_used(&s->cmdfifo)) {
                return;
            }

            /* Command has been received */
            s->do_cmd = 0;
            do_cmd(s);
        } else {
            /*
             * Extra message out bytes received: update cmdfifo_cdb_offset
             * and then switch to commmand phase
             */
            s->cmdfifo_cdb_offset = fifo8_num_used(&s->cmdfifo);
            s->rregs[ESP_RSTAT] = STAT_TC | STAT_CD;
            s->rregs[ESP_RSEQ] = SEQ_CD;
            s->rregs[ESP_RINTR] |= INTR_BS;
            esp_raise_irq(s);
        }
        return;
    }
    if (!s->current_req) {
        return;
    }
    if (s->async_len == 0) {
        /* Defer until data is available.  */
        return;
    }
    if (len > s->async_len) {
        len = s->async_len;
    }
    if (to_device) {
        if (s->dma_memory_read) {
            s->dma_memory_read(s->dma_opaque, s->async_buf, len);
        } else {
            s->pdma_cb = do_dma_pdma_cb;
            esp_raise_drq(s);
            return;
        }
    } else {
        if (s->dma_memory_write) {
            s->dma_memory_write(s->dma_opaque, s->async_buf, len);
        } else {
            /* Adjust TC for any leftover data in the FIFO */
            if (!fifo8_is_empty(&s->fifo)) {
                esp_set_tc(s, esp_get_tc(s) - fifo8_num_used(&s->fifo));
            }

            /* Copy device data to FIFO */
            len = MIN(len, fifo8_num_free(&s->fifo));
            fifo8_push_all(&s->fifo, s->async_buf, len);
            s->async_buf += len;
            s->async_len -= len;
            s->ti_size -= len;

            /*
             * MacOS toolbox uses a TI length of 16 bytes for all commands, so
             * commands shorter than this must be padded accordingly
             */
            if (len < esp_get_tc(s) && esp_get_tc(s) <= ESP_FIFO_SZ) {
                while (fifo8_num_used(&s->fifo) < ESP_FIFO_SZ) {
                    esp_fifo_push(&s->fifo, 0);
                    len++;
                }
            }

            esp_set_tc(s, esp_get_tc(s) - len);
            s->pdma_cb = do_dma_pdma_cb;
            esp_raise_drq(s);

            /* Indicate transfer to FIFO is complete */
            s->rregs[ESP_RSTAT] |= STAT_TC;
            return;
        }
    }
    esp_set_tc(s, esp_get_tc(s) - len);
    s->async_buf += len;
    s->async_len -= len;
    if (to_device) {
        s->ti_size += len;
    } else {
        s->ti_size -= len;
    }
    if (s->async_len == 0) {
        scsi_req_continue(s->current_req);
        /*
         * If there is still data to be read from the device then
         * complete the DMA operation immediately.  Otherwise defer
         * until the scsi layer has completed.
         */
        if (to_device || esp_get_tc(s) != 0 || s->ti_size == 0) {
            return;
        }
    }

    /* Partially filled a scsi buffer. Complete immediately.  */
    esp_dma_done(s);
    esp_lower_drq(s);
}