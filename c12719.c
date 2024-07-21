static void ok_jpg_decode2(ok_jpg_decoder *decoder) {
    ok_jpg *jpg = decoder->jpg;

    // Read header
    uint8_t jpg_header[2];
    if (!ok_read(decoder, jpg_header, 2)) {
        return;
    }
    if (jpg_header[0] != 0xFF || jpg_header[1] != 0xD8) {
        ok_jpg_error(jpg, OK_JPG_ERROR_INVALID, "Invalid signature (not a JPEG file)");
        return;
    }

    while (!decoder->eoi_found) {
        // Read marker
        int marker;
        if (decoder->next_marker != 0) {
            marker = decoder->next_marker;
            decoder->next_marker = 0;
        } else {
            while (true) {
                uint8_t b;
                if (!ok_read(decoder, &b, 1)) {
                    return;
                }
                if (b == 0xFF) {
                    if (!ok_read(decoder, &b, 1)) {
                        return;
                    }
                    if (b != 0 && b != 0xFF) {
                        marker = b;
                        break;
                    }
                }
            }
        }

        bool success = true;
        if (marker == 0xC0 || marker == 0xC1 || marker == 0xC2) {
            // SOF
            decoder->progressive = (marker == 0xC2);
            success = ok_jpg_read_sof(decoder);
            if (success && decoder->info_only) {
                return;
            }
        } else if (marker == 0xC4) {
            // DHT
            success = decoder->info_only ? ok_jpg_skip_segment(decoder) : ok_jpg_read_dht(decoder);
        } else if (marker >= 0xD0 && marker <= 0xD7) {
            decoder->next_marker = marker;
            success = ok_jpg_decode_restart_if_needed(decoder);
            if (success) {
                success = ok_jpg_scan_to_next_marker(decoder);
            }
        } else if (marker == 0xD9) {
            // EOI
            decoder->eoi_found = true;
            if (!decoder->info_only && decoder->progressive) {
                ok_jpg_progressive_finish(decoder);
            }
        } else if (marker == 0xDA) {
            // SOS
            if (!decoder->info_only) {
                success = ok_jpg_read_sos(decoder);
            } else {
                success = ok_jpg_skip_segment(decoder);
                if (success) {
                    success = ok_jpg_scan_to_next_marker(decoder);
                }
            }
        } else if (marker == 0xDB) {
            // DQT
            success = decoder->info_only ? ok_jpg_skip_segment(decoder) : ok_jpg_read_dqt(decoder);
        } else if (marker == 0xDD) {
            // DRI
            success = ok_jpg_read_dri(decoder);
        } else if (marker == 0xE1) {
            // APP1 - EXIF metadata
            success = ok_jpg_read_exif(decoder);
        } else if ((marker >= 0xE0 && marker <= 0xEF) || marker == 0xFE) {
            // APP or Comment
            success = ok_jpg_skip_segment(decoder);
        } else if (marker == 0xFF) {
            // Ignore
        } else {
            ok_jpg_error(jpg, OK_JPG_ERROR_INVALID, "Unsupported or corrupt JPEG");
            success = false;
        }

        if (!success) {
            return;
        }
    }

    if (decoder->num_components == 0) {
        ok_jpg_error(jpg, OK_JPG_ERROR_INVALID, "SOF not found");
    } else {
        for (int i = 0; i < decoder->num_components; i++) {
            if (!decoder->components[i].complete) {
                ok_jpg_error(jpg, OK_JPG_ERROR_INVALID, "Missing JPEG image data");
                break;
            }
        }
    }
}