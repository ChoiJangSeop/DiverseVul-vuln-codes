static void FNAME_DECL(uncompress_row_seg)(const PIXEL * const prev_row,
                                           PIXEL * const cur_row,
                                           int i,
                                           const int end,
                                           const unsigned int bpc_mask)
{
    DECLARE_STATE_VARIABLES;
    DECLARE_CHANNEL_VARIABLES;
    const unsigned int waitmask = bppmask[state->wmidx];
    int stopidx;
    int run_index = 0;
    int run_end;

    spice_assert(end - i > 0);

    if (i == 0) {
        unsigned int codewordlen;

        UNCOMPRESS_PIX_START(&cur_row[i]);
        APPLY_ALL_COMP(UNCOMPRESS_ONE_0);

        if (state->waitcnt) {
            --state->waitcnt;
        } else {
            state->waitcnt = (tabrand(&state->tabrand_seed) & waitmask);
            UPDATE_MODEL(0);
        }
        stopidx = ++i + state->waitcnt;
    } else {
        stopidx = i + state->waitcnt;
    }
    for (;;) {
        while (stopidx < end) {
            for (; i <= stopidx; i++) {
                unsigned int codewordlen;
                RLE_PRED_IMP;
                UNCOMPRESS_PIX_START(&cur_row[i]);
                APPLY_ALL_COMP(UNCOMPRESS_ONE);
            }

            UPDATE_MODEL(stopidx);

            stopidx = i + (tabrand(&state->tabrand_seed) & waitmask);
        }

        for (; i < end; i++) {
            unsigned int codewordlen;
            RLE_PRED_IMP;
            UNCOMPRESS_PIX_START(&cur_row[i]);
            APPLY_ALL_COMP(UNCOMPRESS_ONE);
        }

        state->waitcnt = stopidx - end;

        return;

do_run:
        state->waitcnt = stopidx - i;
        run_index = i;
        run_end = i + decode_state_run(encoder, state);

        for (; i < run_end; i++) {
            UNCOMPRESS_PIX_START(&cur_row[i]);
            COPY_PIXEL(&cur_row[i], &cur_row[i - 1]);
        }

        if (i == end) {
            return;
        }

        stopidx = i + state->waitcnt;
    }
}