static int acurite_00275rm_decode(r_device *decoder, bitbuffer_t *bitbuffer)
{
    int result = 0;
    bitbuffer_invert(bitbuffer);

    // This sensor repeats a signal three times. Combine as fallback.
    uint8_t *b_rows[3] = {0};
    int n_rows         = 0;
    for (int row = 0; row < bitbuffer->num_rows; ++row) {
        if (n_rows < 3 && bitbuffer->bits_per_row[row] == 88) {
            b_rows[n_rows] = bitbuffer->bb[row];
            n_rows++;
        }
    }

    // Combine signal if exactly three repeats were found
    if (n_rows == 3) {
        uint8_t *b = bitbuffer->bb[bitbuffer->num_rows];
        for (int i = 0; i < 11; ++i) {
            // The majority bit count wins
            b[i] = (b_rows[0][i] & b_rows[1][i]) |
                    (b_rows[1][i] & b_rows[2][i]) |
                    (b_rows[2][i] & b_rows[0][i]);
        }
        bitbuffer->bits_per_row[bitbuffer->num_rows] = 88;
        bitbuffer->num_rows += 1;
    }

    // Output the first valid row
    for (int row = 0; row < bitbuffer->num_rows; ++row) {
        if (bitbuffer->bits_per_row[row] != 88) {
            result = DECODE_ABORT_LENGTH;
            continue; // return DECODE_ABORT_LENGTH;
        }
        uint8_t *b = bitbuffer->bb[row];

        // Check CRC
        if (crc16lsb(b, 11, 0x00b2, 0x00d0) != 0) {
            decoder_log_bitrow(decoder, 1, __func__, b, 11 * 8, "sensor bad CRC");
            result = DECODE_FAIL_MIC;
            continue; // return DECODE_FAIL_MIC;
        }

        //  Decode common fields
        int id          = (b[0] << 16) | (b[1] << 8) | b[3];
        int battery_low = (b[2] & 0x40) == 0;
        int model_flag  = (b[2] & 1);
        float tempc     = ((b[4] << 4) | (b[5] >> 4)) * 0.1 - 100;
        int probe       = b[5] & 3;
        int humidity    = ((b[6] & 0x1f) << 2) | (b[7] >> 6);

        //  Water probe (detects water leak)
        int water = (b[7] & 0x0f) == 15; // valid only if (probe == 1)
        //  Soil probe (detects temperature)
        float ptempc = (((b[7] & 0x0f) << 8) | b[8]) * 0.1 - 100; // valid only if (probe == 2 || probe == 3)
        //  Spot probe (detects temperature and humidity)
        int phumidity = b[9] & 0x7f; // valid only if (probe == 3)

        /* clang-format off */
        data_t *data = data_make(
                "model",            "",             DATA_STRING,    model_flag ? "Acurite-00275rm" : "Acurite-00276rm",
                "subtype",          "Probe",        DATA_INT,       probe,
                "id",               "",             DATA_INT,       id,
                "battery_ok",       "Battery",      DATA_INT,       !battery_low,
                "temperature_C",    "Celsius",      DATA_FORMAT,    "%.1f C",  DATA_DOUBLE, tempc,
                "humidity",         "Humidity",     DATA_FORMAT,    "%u %%", DATA_INT,      humidity,
                "water",            "",             DATA_COND, probe == 1, DATA_INT,        water,
                "temperature_1_C",  "Celsius",      DATA_COND, probe == 2, DATA_FORMAT, "%.1f C",   DATA_DOUBLE, ptempc,
                "temperature_1_C",  "Celsius",      DATA_COND, probe == 3, DATA_FORMAT, "%.1f C",   DATA_DOUBLE, ptempc,
                "humidity_1",       "Humidity",     DATA_COND, probe == 3, DATA_FORMAT, "%u %%",    DATA_INT,    phumidity,
                "mic",              "Integrity",    DATA_STRING,    "CRC",
                NULL);
        /* clang-format on */

        decoder_output_data(decoder, data);

        return 1;
    }
    // Only returns the latest result, but better than nothing.
    return result;
}