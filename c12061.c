static int read_tfra(MOVContext *mov, AVIOContext *f)
{
    MOVFragmentIndex* index = NULL;
    int version, fieldlength, i, j;
    int64_t pos = avio_tell(f);
    uint32_t size = avio_rb32(f);
    void *tmp;

    if (avio_rb32(f) != MKBETAG('t', 'f', 'r', 'a')) {
        return 1;
    }
    av_log(mov->fc, AV_LOG_VERBOSE, "found tfra\n");
    index = av_mallocz(sizeof(MOVFragmentIndex));
    if (!index) {
        return AVERROR(ENOMEM);
    }

    tmp = av_realloc_array(mov->fragment_index_data,
                           mov->fragment_index_count + 1,
                           sizeof(MOVFragmentIndex*));
    if (!tmp) {
        av_freep(&index);
        return AVERROR(ENOMEM);
    }
    mov->fragment_index_data = tmp;
    mov->fragment_index_data[mov->fragment_index_count++] = index;

    version = avio_r8(f);
    avio_rb24(f);
    index->track_id = avio_rb32(f);
    fieldlength = avio_rb32(f);
    index->item_count = avio_rb32(f);
    index->items = av_mallocz_array(
            index->item_count, sizeof(MOVFragmentIndexItem));
    if (!index->items) {
        index->item_count = 0;
        return AVERROR(ENOMEM);
    }
    for (i = 0; i < index->item_count; i++) {
        int64_t time, offset;
        if (version == 1) {
            time   = avio_rb64(f);
            offset = avio_rb64(f);
        } else {
            time   = avio_rb32(f);
            offset = avio_rb32(f);
        }
        index->items[i].time = time;
        index->items[i].moof_offset = offset;
        for (j = 0; j < ((fieldlength >> 4) & 3) + 1; j++)
            avio_r8(f);
        for (j = 0; j < ((fieldlength >> 2) & 3) + 1; j++)
            avio_r8(f);
        for (j = 0; j < ((fieldlength >> 0) & 3) + 1; j++)
            avio_r8(f);
    }

    avio_seek(f, pos + size, SEEK_SET);
    return 0;
}