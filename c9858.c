static void do_dma_pdma_cb(ESPState *s)
{
    int to_device = ((s->rregs[ESP_RSTAT] & 7) == STAT_DO);
    int len;
    uint32_t n;

    if (s->do_cmd) {
        s->ti_size = 0;
        s->do_cmd = 0;
        do_cmd(s);
        esp_lower_drq(s);
        return;
    }

    if (to_device) {
        /* Copy FIFO data to device */
        len = MIN(s->async_len, ESP_FIFO_SZ);
        len = MIN(len, fifo8_num_used(&s->fifo));
        memcpy(s->async_buf, fifo8_pop_buf(&s->fifo, len, &n), len);
        s->async_buf += n;
        s->async_len -= n;
        s->ti_size += n;

        if (n < len) {
            /* Unaligned accesses can cause FIFO wraparound */
            len = len - n;
            memcpy(s->async_buf, fifo8_pop_buf(&s->fifo, len, &n), len);
            s->async_buf += n;
            s->async_len -= n;
            s->ti_size += n;
        }

        if (s->async_len == 0) {
            scsi_req_continue(s->current_req);
            return;
        }

        if (esp_get_tc(s) == 0) {
            esp_lower_drq(s);
            esp_dma_done(s);
        }

        return;
    } else {
        if (s->async_len == 0) {
            if (s->current_req) {
                /* Defer until the scsi layer has completed */
                scsi_req_continue(s->current_req);
                s->data_in_ready = false;
            }
            return;
        }

        if (esp_get_tc(s) != 0) {
            /* Copy device data to FIFO */
            len = MIN(s->async_len, esp_get_tc(s));
            len = MIN(len, fifo8_num_free(&s->fifo));
            fifo8_push_all(&s->fifo, s->async_buf, len);
            s->async_buf += len;
            s->async_len -= len;
            s->ti_size -= len;
            esp_set_tc(s, esp_get_tc(s) - len);

            if (esp_get_tc(s) == 0) {
                /* Indicate transfer to FIFO is complete */
                 s->rregs[ESP_RSTAT] |= STAT_TC;
            }
            return;
        }

        /* Partially filled a scsi buffer. Complete immediately.  */
        esp_lower_drq(s);
        esp_dma_done(s);
    }
}