static void esp_do_dma(ESPState *s)
{
    uint32_t len;
    int to_device;

    len = s->dma_left;
    if (s->do_cmd) {
        trace_esp_do_dma(s->cmdlen, len);
        s->dma_memory_read(s->dma_opaque, &s->cmdbuf[s->cmdlen], len);
        return;
    }
    if (s->async_len == 0) {
        /* Defer until data is available.  */
        return;
    }
    if (len > s->async_len) {
        len = s->async_len;
    }
    to_device = (s->ti_size < 0);
    if (to_device) {
        s->dma_memory_read(s->dma_opaque, s->async_buf, len);
    } else {
        s->dma_memory_write(s->dma_opaque, s->async_buf, len);
    }
    s->dma_left -= len;
    s->async_buf += len;
    s->async_len -= len;
    if (to_device)
        s->ti_size += len;
    else
        s->ti_size -= len;
    if (s->async_len == 0) {
        scsi_req_continue(s->current_req);
        /* If there is still data to be read from the device then
           complete the DMA operation immediately.  Otherwise defer
           until the scsi layer has completed.  */
        if (to_device || s->dma_left != 0 || s->ti_size == 0) {
            return;
        }
    }

    /* Partially filled a scsi buffer. Complete immediately.  */
    esp_dma_done(s);
}