static void sdhci_do_adma(SDHCIState *s)
{
    unsigned int begin, length;
    const uint16_t block_size = s->blksize & BLOCK_SIZE_MASK;
    ADMADescr dscr = {};
    int i;

    if (s->trnmod & SDHC_TRNS_BLK_CNT_EN && !s->blkcnt) {
        /* Stop Multiple Transfer */
        sdhci_end_transfer(s);
        return;
    }

    for (i = 0; i < SDHC_ADMA_DESCS_PER_DELAY; ++i) {
        s->admaerr &= ~SDHC_ADMAERR_LENGTH_MISMATCH;

        get_adma_description(s, &dscr);
        trace_sdhci_adma_loop(dscr.addr, dscr.length, dscr.attr);

        if ((dscr.attr & SDHC_ADMA_ATTR_VALID) == 0) {
            /* Indicate that error occurred in ST_FDS state */
            s->admaerr &= ~SDHC_ADMAERR_STATE_MASK;
            s->admaerr |= SDHC_ADMAERR_STATE_ST_FDS;

            /* Generate ADMA error interrupt */
            if (s->errintstsen & SDHC_EISEN_ADMAERR) {
                s->errintsts |= SDHC_EIS_ADMAERR;
                s->norintsts |= SDHC_NIS_ERR;
            }

            sdhci_update_irq(s);
            return;
        }

        length = dscr.length ? dscr.length : 64 * KiB;

        switch (dscr.attr & SDHC_ADMA_ATTR_ACT_MASK) {
        case SDHC_ADMA_ATTR_ACT_TRAN:  /* data transfer */
            if (s->trnmod & SDHC_TRNS_READ) {
                while (length) {
                    if (s->data_count == 0) {
                        sdbus_read_data(&s->sdbus, s->fifo_buffer, block_size);
                    }
                    begin = s->data_count;
                    if ((length + begin) < block_size) {
                        s->data_count = length + begin;
                        length = 0;
                     } else {
                        s->data_count = block_size;
                        length -= block_size - begin;
                    }
                    dma_memory_write(s->dma_as, dscr.addr,
                                     &s->fifo_buffer[begin],
                                     s->data_count - begin);
                    dscr.addr += s->data_count - begin;
                    if (s->data_count == block_size) {
                        s->data_count = 0;
                        if (s->trnmod & SDHC_TRNS_BLK_CNT_EN) {
                            s->blkcnt--;
                            if (s->blkcnt == 0) {
                                break;
                            }
                        }
                    }
                }
            } else {
                while (length) {
                    begin = s->data_count;
                    if ((length + begin) < block_size) {
                        s->data_count = length + begin;
                        length = 0;
                     } else {
                        s->data_count = block_size;
                        length -= block_size - begin;
                    }
                    dma_memory_read(s->dma_as, dscr.addr,
                                    &s->fifo_buffer[begin],
                                    s->data_count - begin);
                    dscr.addr += s->data_count - begin;
                    if (s->data_count == block_size) {
                        sdbus_write_data(&s->sdbus, s->fifo_buffer, block_size);
                        s->data_count = 0;
                        if (s->trnmod & SDHC_TRNS_BLK_CNT_EN) {
                            s->blkcnt--;
                            if (s->blkcnt == 0) {
                                break;
                            }
                        }
                    }
                }
            }
            s->admasysaddr += dscr.incr;
            break;
        case SDHC_ADMA_ATTR_ACT_LINK:   /* link to next descriptor table */
            s->admasysaddr = dscr.addr;
            trace_sdhci_adma("link", s->admasysaddr);
            break;
        default:
            s->admasysaddr += dscr.incr;
            break;
        }

        if (dscr.attr & SDHC_ADMA_ATTR_INT) {
            trace_sdhci_adma("interrupt", s->admasysaddr);
            if (s->norintstsen & SDHC_NISEN_DMA) {
                s->norintsts |= SDHC_NIS_DMA;
            }

            if (sdhci_update_irq(s) && !(dscr.attr & SDHC_ADMA_ATTR_END)) {
                /* IRQ delivered, reschedule current transfer */
                break;
            }
        }

        /* ADMA transfer terminates if blkcnt == 0 or by END attribute */
        if (((s->trnmod & SDHC_TRNS_BLK_CNT_EN) &&
                    (s->blkcnt == 0)) || (dscr.attr & SDHC_ADMA_ATTR_END)) {
            trace_sdhci_adma_transfer_completed();
            if (length || ((dscr.attr & SDHC_ADMA_ATTR_END) &&
                (s->trnmod & SDHC_TRNS_BLK_CNT_EN) &&
                s->blkcnt != 0)) {
                trace_sdhci_error("SD/MMC host ADMA length mismatch");
                s->admaerr |= SDHC_ADMAERR_LENGTH_MISMATCH |
                        SDHC_ADMAERR_STATE_ST_TFR;
                if (s->errintstsen & SDHC_EISEN_ADMAERR) {
                    trace_sdhci_error("Set ADMA error flag");
                    s->errintsts |= SDHC_EIS_ADMAERR;
                    s->norintsts |= SDHC_NIS_ERR;
                }

                sdhci_update_irq(s);
            }
            sdhci_end_transfer(s);
            return;
        }

    }

    /* we have unfinished business - reschedule to continue ADMA */
    timer_mod(s->transfer_timer,
                   qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + SDHC_TRANSFER_DELAY);
}