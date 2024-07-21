static void ok_png_decode2(ok_png_decoder *decoder) {
    ok_png *png = decoder->png;

    uint8_t png_header[8];
    if (!ok_read(decoder, png_header, sizeof(png_header))) {
        return;
    }
    uint8_t png_signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    if (memcmp(png_header, png_signature, 8) != 0) {
        ok_png_error(decoder->png, OK_PNG_ERROR_INVALID, "Invalid signature (not a PNG file)");
        return;
    }

    // When info_only is true, we only care about the IHDR chunk and whether or not
    // the tRNS chunk exists.
    bool info_only = (decoder->decode_flags & OK_PNG_INFO_ONLY) != 0;
    bool hdr_found = false;
    bool end_found = false;
    while (!end_found) {
        uint8_t chunk_header[8];
        uint8_t chunk_footer[4];
        if (!ok_read(decoder, chunk_header, sizeof(chunk_header))) {
            return;
        }
        const uint32_t chunk_length = readBE32(chunk_header);
        const uint32_t chunk_type = readBE32(chunk_header + 4);
        bool success = false;

        if (!hdr_found && chunk_type != OK_PNG_CHUNK_CGBI && chunk_type != OK_PNG_CHUNK_IHDR) {
            ok_png_error(png, OK_PNG_ERROR_INVALID, "IHDR chunk must appear first");
            return;
        }
        if (chunk_type == OK_PNG_CHUNK_IHDR) {
            hdr_found = true;
            success = ok_png_read_header(decoder, chunk_length);
            if (success && info_only) {
                // If the png has alpha, then we have all the info we need.
                // Otherwise, continue scanning to see if the tRNS chunk exists.
                if (png->has_alpha) {
                    return;
                }
            }
        } else if (chunk_type == OK_PNG_CHUNK_CGBI) {
            success = ok_seek(decoder, (long)chunk_length);
            decoder->is_ios_format = true;
        } else if (chunk_type == OK_PNG_CHUNK_PLTE && !info_only) {
            success = ok_png_read_palette(decoder, chunk_length);
        } else if (chunk_type == OK_PNG_CHUNK_TRNS) {
            if (info_only) {
                // No need to parse this chunk, we have all the info we need.
                png->has_alpha = true;
                return;
            } else {
                success = ok_png_read_transparency(decoder, chunk_length);
            }
        } else if (chunk_type == OK_PNG_CHUNK_IDAT) {
            if (info_only) {
                // Both IHDR and tRNS must come before IDAT, so we have all the info we need.
                return;
            }
            success = ok_png_read_data(decoder, chunk_length);
        } else if (chunk_type == OK_PNG_CHUNK_IEND) {
            success = ok_seek(decoder, (long)chunk_length);
            end_found = true;
        } else {
            // Ignore this chunk
            success = ok_seek(decoder, (long)chunk_length);
        }

        if (!success) {
            return;
        }

        // Read the footer (CRC) and ignore it
        if (!ok_read(decoder, chunk_footer, sizeof(chunk_footer))) {
            return;
        }
    }

    // Sanity check
    if (!decoder->decoding_completed) {
        ok_png_error(png, OK_PNG_ERROR_INVALID, "Missing imaga data");
    }
}