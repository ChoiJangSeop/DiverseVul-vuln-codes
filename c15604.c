int create_dpb(davs2_mgr_t *mgr)
{
    davs2_seq_t *seq = &mgr->seq_info;
    uint8_t      *mem_ptr = NULL;
    size_t        mem_size = 0;
    int i;

    mgr->dpbsize = mgr->num_decoders + seq->picture_reorder_delay + 16;  /// !!! FIXME: decide dpb buffer size ?
    mgr->dpbsize += 8;  // FIXME: Ҫ

    mem_size = mgr->dpbsize * sizeof(davs2_frame_t *)
        + davs2_frame_get_size(seq->i_enc_width, seq->i_enc_height, seq->head.chroma_format, 1) * mgr->dpbsize
        + davs2_frame_get_size(seq->i_enc_width, seq->i_enc_height, seq->head.chroma_format, 0)
        + CACHE_LINE_SIZE * (mgr->dpbsize + 2);

    mem_ptr = (uint8_t *)davs2_malloc(mem_size);
    if (mem_ptr == NULL) {
        return -1;
    }

    mgr->dpb = (davs2_frame_t **)mem_ptr;
    mem_ptr += mgr->dpbsize * sizeof(davs2_frame_t *);
    ALIGN_POINTER(mem_ptr);

    for (i = 0; i < mgr->dpbsize; i++) {
        mgr->dpb[i] = davs2_frame_new(seq->i_enc_width, seq->i_enc_height, seq->head.chroma_format, &mem_ptr, 1);
        ALIGN_POINTER(mem_ptr);

        if (mgr->dpb[i] == NULL) {
            return -1;
        }
    }

    return 0;
}