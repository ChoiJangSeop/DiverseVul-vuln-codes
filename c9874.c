static void do_busid_cmd(ESPState *s, uint8_t busid)
{
    uint32_t n, cmdlen;
    int32_t datalen;
    int lun;
    SCSIDevice *current_lun;
    uint8_t *buf;

    trace_esp_do_busid_cmd(busid);
    lun = busid & 7;
    cmdlen = fifo8_num_used(&s->cmdfifo);
    buf = (uint8_t *)fifo8_pop_buf(&s->cmdfifo, cmdlen, &n);

    current_lun = scsi_device_find(&s->bus, 0, s->current_dev->id, lun);
    s->current_req = scsi_req_new(current_lun, 0, lun, buf, s);
    datalen = scsi_req_enqueue(s->current_req);
    s->ti_size = datalen;
    fifo8_reset(&s->cmdfifo);
    if (datalen != 0) {
        s->rregs[ESP_RSTAT] = STAT_TC;
        s->rregs[ESP_RSEQ] = SEQ_CD;
        s->ti_cmd = 0;
        esp_set_tc(s, 0);
        if (datalen > 0) {
            /*
             * Switch to DATA IN phase but wait until initial data xfer is
             * complete before raising the command completion interrupt
             */
            s->data_in_ready = false;
            s->rregs[ESP_RSTAT] |= STAT_DI;
        } else {
            s->rregs[ESP_RSTAT] |= STAT_DO;
            s->rregs[ESP_RINTR] |= INTR_BS | INTR_FC;
            esp_raise_irq(s);
            esp_lower_drq(s);
        }
        scsi_req_continue(s->current_req);
        return;
    }
}