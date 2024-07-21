static void sdhci_send_command(SDHCIState *s)
{
    SDRequest request;
    uint8_t response[16];
    int rlen;

    s->errintsts = 0;
    s->acmd12errsts = 0;
    request.cmd = s->cmdreg >> 8;
    request.arg = s->argument;

    trace_sdhci_send_command(request.cmd, request.arg);
    rlen = sdbus_do_command(&s->sdbus, &request, response);

    if (s->cmdreg & SDHC_CMD_RESPONSE) {
        if (rlen == 4) {
            s->rspreg[0] = ldl_be_p(response);
            s->rspreg[1] = s->rspreg[2] = s->rspreg[3] = 0;
            trace_sdhci_response4(s->rspreg[0]);
        } else if (rlen == 16) {
            s->rspreg[0] = ldl_be_p(&response[11]);
            s->rspreg[1] = ldl_be_p(&response[7]);
            s->rspreg[2] = ldl_be_p(&response[3]);
            s->rspreg[3] = (response[0] << 16) | (response[1] << 8) |
                            response[2];
            trace_sdhci_response16(s->rspreg[3], s->rspreg[2],
                                   s->rspreg[1], s->rspreg[0]);
        } else {
            trace_sdhci_error("timeout waiting for command response");
            if (s->errintstsen & SDHC_EISEN_CMDTIMEOUT) {
                s->errintsts |= SDHC_EIS_CMDTIMEOUT;
                s->norintsts |= SDHC_NIS_ERR;
            }
        }

        if (!(s->quirks & SDHCI_QUIRK_NO_BUSY_IRQ) &&
            (s->norintstsen & SDHC_NISEN_TRSCMP) &&
            (s->cmdreg & SDHC_CMD_RESPONSE) == SDHC_CMD_RSP_WITH_BUSY) {
            s->norintsts |= SDHC_NIS_TRSCMP;
        }
    }

    if (s->norintstsen & SDHC_NISEN_CMDCMP) {
        s->norintsts |= SDHC_NIS_CMDCMP;
    }

    sdhci_update_irq(s);

    if (s->blksize && (s->cmdreg & SDHC_CMD_DATA_PRESENT)) {
        s->data_count = 0;
        sdhci_data_transfer(s);
    }
}