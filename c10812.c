MOBI_RET mobi_parse_huff(MOBIHuffCdic *huffcdic, const MOBIPdbRecord *record) {
    MOBIBuffer *buf = mobi_buffer_init_null(record->data, record->size);
    if (buf == NULL) {
        debug_print("%s\n", "Memory allocation failed");
        return MOBI_MALLOC_FAILED;
    }
    char huff_magic[5];
    mobi_buffer_getstring(huff_magic, buf, 4);
    const size_t header_length = mobi_buffer_get32(buf);
    if (strncmp(huff_magic, HUFF_MAGIC, 4) != 0 || header_length < HUFF_HEADER_LEN) {
        debug_print("HUFF wrong magic: %s\n", huff_magic);
        mobi_buffer_free_null(buf);
        return MOBI_DATA_CORRUPT;
    }
    const size_t data1_offset = mobi_buffer_get32(buf);
    const size_t data2_offset = mobi_buffer_get32(buf);
    /* skip little-endian table offsets */
    mobi_buffer_setpos(buf, data1_offset);
    if (buf->offset + (256 * 4) > buf->maxlen) {
        debug_print("%s", "HUFF data1 too short\n");
        mobi_buffer_free_null(buf);
        return MOBI_DATA_CORRUPT;
    }
    /* read 256 indices from data1 big-endian */
    for (int i = 0; i < 256; i++) {
        huffcdic->table1[i] = mobi_buffer_get32(buf);
    }
    mobi_buffer_setpos(buf, data2_offset);
    if (buf->offset + (64 * 4) > buf->maxlen) {
        debug_print("%s", "HUFF data2 too short\n");
        mobi_buffer_free_null(buf);
        return MOBI_DATA_CORRUPT;
    }
    /* read 32 mincode-maxcode pairs from data2 big-endian */
    huffcdic->mincode_table[0] = 0;
    huffcdic->maxcode_table[0] = 0xFFFFFFFF;
    for (int i = 1; i < 33; i++) {
        const uint32_t mincode = mobi_buffer_get32(buf);
        const uint32_t maxcode = mobi_buffer_get32(buf);
        huffcdic->mincode_table[i] =  mincode << (32 - i);
        huffcdic->maxcode_table[i] =  ((maxcode + 1) << (32 - i)) - 1;
    }
    mobi_buffer_free_null(buf);
    return MOBI_SUCCESS;
}