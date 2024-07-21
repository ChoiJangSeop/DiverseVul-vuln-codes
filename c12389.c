static bool ok_jpg_read_dht(ok_jpg_decoder *decoder) {
    // JPEG spec: Table B.5
    ok_jpg *jpg = decoder->jpg;
    uint8_t buffer[17];
    if (!ok_read(decoder, buffer, 2)) {
        return false;
    }
    int length = readBE16(buffer) - 2;
    while (length >= 17) {
        if (!ok_read(decoder, buffer, 17)) {
            return false;
        }
        length -= 17;

        int Tc = buffer[0] >> 4;
        int Th = buffer[0] & 0x0f;
        if (Tc > 1 || Th > 3) {
            ok_jpg_error(jpg, OK_JPG_ERROR_INVALID, "Invalid JPEG (Bad DHT Tc/Th)");
            return false;
        }
        ok_jpg_huffman_table *tables = (Tc == 0 ? decoder->dc_huffman_tables :
                                        decoder->ac_huffman_tables);
        ok_jpg_huffman_table *table = tables + Th;
        ok_jpg_generate_huffman_table(table, buffer);
        if (table->count > 0) {
            if (table->count > 256 || table->count > length) {
                ok_jpg_error(jpg, OK_JPG_ERROR_INVALID, "Invalid DHT segment length");
                return false;
            }
            if (!ok_read(decoder, table->val, (size_t)table->count)) {
                return false;
            }
            length -= table->count;
        }
        bool is_ac_table = Tc == 1;
        ok_jpg_generate_huffman_table_lookups(table, is_ac_table);
    }
    if (length != 0) {
        ok_jpg_error(jpg, OK_JPG_ERROR_INVALID, "Invalid DHT segment length");
        return false;
    } else {
        return true;
    }
}