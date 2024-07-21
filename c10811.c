static MOBI_RET mobi_decompress_huffman_internal(MOBIBuffer *buf_out, MOBIBuffer *buf_in, const MOBIHuffCdic *huffcdic, size_t depth) {
    if (depth > MOBI_HUFFMAN_MAXDEPTH) {
        debug_print("Too many levels of recursion: %zu\n", depth);
        return MOBI_DATA_CORRUPT;
    }
    MOBI_RET ret = MOBI_SUCCESS;
    int8_t bitcount = 32;
    /* this cast should be safe: max record size is 4096 */
    int bitsleft = (int) (buf_in->maxlen * 8);
    uint8_t code_length = 0;
    uint64_t buffer = mobi_buffer_fill64(buf_in);
    while (ret == MOBI_SUCCESS) {
        if (bitcount <= 0) {
            bitcount += 32;
            buffer = mobi_buffer_fill64(buf_in);
        }
        uint32_t code = (buffer >> bitcount) & 0xffffffffU;
        /* lookup code in table1 */
        uint32_t t1 = huffcdic->table1[code >> 24];
        /* get maxcode and codelen from t1 */
        code_length = t1 & 0x1f;
        uint32_t maxcode = (((t1 >> 8) + 1) << (32 - code_length)) - 1;
        /* check termination bit */
        if (!(t1 & 0x80)) {
            /* get offset from mincode, maxcode tables */
            while (code < huffcdic->mincode_table[code_length]) {
                code_length++;
            }
            maxcode = huffcdic->maxcode_table[code_length];
        }
        bitcount -= code_length;
        bitsleft -= code_length;
        if (bitsleft < 0) {
            break;
        }
        /* get index for symbol offset */
        uint32_t index = (uint32_t) (maxcode - code) >> (32 - code_length);
        /* check which part of cdic to use */
        uint16_t cdic_index = (uint16_t) ((uint32_t)index >> huffcdic->code_length);
        if (index >= huffcdic->index_count) {
            debug_print("Wrong symbol offsets index: %u\n", index);
            return MOBI_DATA_CORRUPT;
        }
        /* get offset */
        uint32_t offset = huffcdic->symbol_offsets[index];
        uint32_t symbol_length = (uint32_t) huffcdic->symbols[cdic_index][offset] << 8 | (uint32_t) huffcdic->symbols[cdic_index][offset + 1];
        /* 1st bit is is_decompressed flag */
        int is_decompressed = symbol_length >> 15;
        /* get rid of flag */
        symbol_length &= 0x7fff;
        if (is_decompressed) {
            /* symbol is at (offset + 2), 2 bytes used earlier for symbol length */
            mobi_buffer_addraw(buf_out, (huffcdic->symbols[cdic_index] + offset + 2), symbol_length);
            ret = buf_out->error;
        } else {
            /* symbol is compressed */
            /* TODO cache uncompressed symbols? */
            MOBIBuffer buf_sym;
            buf_sym.data = huffcdic->symbols[cdic_index] + offset + 2;
            buf_sym.offset = 0;
            buf_sym.maxlen = symbol_length;
            buf_sym.error = MOBI_SUCCESS;
            ret = mobi_decompress_huffman_internal(buf_out, &buf_sym, huffcdic, depth + 1);
        }
    }
    return ret;
}