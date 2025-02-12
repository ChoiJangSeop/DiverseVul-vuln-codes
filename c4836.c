tight_detect_smooth_image(VncState *vs, int w, int h)
{
    unsigned int errors;
    int compression = vs->tight.compression;
    int quality = vs->tight.quality;

    if (!vs->vd->lossy) {
        return 0;
    }

    if (ds_get_bytes_per_pixel(vs->ds) == 1 ||
        vs->clientds.pf.bytes_per_pixel == 1 ||
        w < VNC_TIGHT_DETECT_MIN_WIDTH || h < VNC_TIGHT_DETECT_MIN_HEIGHT) {
        return 0;
    }

    if (vs->tight.quality != (uint8_t)-1) {
        if (w * h < VNC_TIGHT_JPEG_MIN_RECT_SIZE) {
            return 0;
        }
    } else {
        if (w * h < tight_conf[compression].gradient_min_rect_size) {
            return 0;
        }
    }

    if (vs->clientds.pf.bytes_per_pixel == 4) {
        if (vs->tight.pixel24) {
            errors = tight_detect_smooth_image24(vs, w, h);
            if (vs->tight.quality != (uint8_t)-1) {
                return (errors < tight_conf[quality].jpeg_threshold24);
            }
            return (errors < tight_conf[compression].gradient_threshold24);
        } else {
            errors = tight_detect_smooth_image32(vs, w, h);
        }
    } else {
        errors = tight_detect_smooth_image16(vs, w, h);
    }
    if (quality != -1) {
        return (errors < tight_conf[quality].jpeg_threshold);
    }
    return (errors < tight_conf[compression].gradient_threshold);
}