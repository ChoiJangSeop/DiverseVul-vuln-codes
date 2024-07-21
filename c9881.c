static int esp_select(ESPState *s)
{
    int target;

    target = s->wregs[ESP_WBUSID] & BUSID_DID;

    s->ti_size = 0;
    fifo8_reset(&s->fifo);

    if (s->current_req) {
        /* Started a new command before the old one finished.  Cancel it.  */
        scsi_req_cancel(s->current_req);
        s->async_len = 0;
    }

    s->current_dev = scsi_device_find(&s->bus, 0, target, 0);
    if (!s->current_dev) {
        /* No such drive */
        s->rregs[ESP_RSTAT] = 0;
        s->rregs[ESP_RINTR] |= INTR_DC;
        s->rregs[ESP_RSEQ] = SEQ_0;
        esp_raise_irq(s);
        return -1;
    }

    /*
     * Note that we deliberately don't raise the IRQ here: this will be done
     * either in do_busid_cmd() for DATA OUT transfers or by the deferred
     * IRQ mechanism in esp_transfer_data() for DATA IN transfers
     */
    s->rregs[ESP_RINTR] |= INTR_FC;
    s->rregs[ESP_RSEQ] = SEQ_CD;
    return 0;
}