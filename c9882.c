static void handle_satn(ESPState *s)
{
    int32_t cmdlen;

    if (s->dma && !s->dma_enabled) {
        s->dma_cb = handle_satn;
        return;
    }
    s->pdma_cb = satn_pdma_cb;
    cmdlen = get_cmd(s, ESP_CMDFIFO_SZ);
    if (cmdlen > 0) {
        s->cmdfifo_cdb_offset = 1;
        do_cmd(s);
    } else if (cmdlen == 0) {
        s->do_cmd = 1;
        /* Target present, but no cmd yet - switch to command phase */
        s->rregs[ESP_RSEQ] = SEQ_CD;
        s->rregs[ESP_RSTAT] = STAT_CD;
    }
}