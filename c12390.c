static void ok_jpg_generate_huffman_table(ok_jpg_huffman_table *huff, const uint8_t *bits) {
    // JPEG spec: "Generate_size_table"
    int k = 0;
    for (uint8_t i = 1; i <= 16; i++) {
        for (int j = 1; j <= bits[i]; j++) {
            huff->size[k++] = i;
        }
    }
    huff->size[k] = 0;
    huff->count = k;

    // JPEG spec: "Generate_code_table"
    k = 0;
    uint16_t code = 0;
    int si = huff->size[0];
    while (true) {
        huff->code[k] = code;
        code++;
        k++;
        int si2 = huff->size[k];
        if (si2 == 0) {
            break;
        }
        if (si2 > si) {
            code <<= (si2 - si);
            si = si2;
        }
    }

    // JPEG spec: "Decoder_tables"
    int j = 0;
    for (int i = 0; i < 16; i++) {
        if (bits[i + 1] == 0) {
            huff->maxcode[i] = -1;
        } else {
            huff->valptr[i] = j;
            huff->mincode[i] = huff->code[j];
            j += bits[i + 1];
            huff->maxcode[i] = huff->code[j - 1];
            if (i >= HUFFMAN_LOOKUP_SIZE_BITS) {
                huff->maxcode[i] = (huff->maxcode[i] << (15 - i)) | ((1 << (15 - i)) - 1);
            }
        }
    }
}