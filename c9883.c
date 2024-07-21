static void handle_s_without_atn(ESPState *s)
{
    int32_t cmdlen;

    if (s->dma && !s->dma_enabled) {
        s->dma_cb = handle_s_without_atn;
        return;
    }
    s->pdma_cb = s_without_satn_pdma_cb;
    cmdlen = get_cmd(s, ESP_CMDFIFO_SZ);
    if (cmdlen > 0) {
        s->cmdfifo_cdb_offset = 0;
        do_busid_cmd(s, 0);
    } else if (cmdlen == 0) {
        s->do_cmd = 1;
        /* Target present, but no cmd yet - switch to command phase */
        s->rregs[ESP_RSEQ] = SEQ_CD;
        s->rregs[ESP_RSTAT] = STAT_CD;
    }
}