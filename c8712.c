static void set_encodings(VncState *vs, int32_t *encodings, size_t n_encodings)
{
    int i;
    unsigned int enc = 0;

    vs->features = 0;
    vs->vnc_encoding = 0;
    vs->tight.compression = 9;
    vs->tight.quality = -1; /* Lossless by default */
    vs->absolute = -1;

    /*
     * Start from the end because the encodings are sent in order of preference.
     * This way the preferred encoding (first encoding defined in the array)
     * will be set at the end of the loop.
     */
    for (i = n_encodings - 1; i >= 0; i--) {
        enc = encodings[i];
        switch (enc) {
        case VNC_ENCODING_RAW:
            vs->vnc_encoding = enc;
            break;
        case VNC_ENCODING_COPYRECT:
            vs->features |= VNC_FEATURE_COPYRECT_MASK;
            break;
        case VNC_ENCODING_HEXTILE:
            vs->features |= VNC_FEATURE_HEXTILE_MASK;
            vs->vnc_encoding = enc;
            break;
        case VNC_ENCODING_TIGHT:
            vs->features |= VNC_FEATURE_TIGHT_MASK;
            vs->vnc_encoding = enc;
            break;
#ifdef CONFIG_VNC_PNG
        case VNC_ENCODING_TIGHT_PNG:
            vs->features |= VNC_FEATURE_TIGHT_PNG_MASK;
            vs->vnc_encoding = enc;
            break;
#endif
        case VNC_ENCODING_ZLIB:
            vs->features |= VNC_FEATURE_ZLIB_MASK;
            vs->vnc_encoding = enc;
            break;
        case VNC_ENCODING_ZRLE:
            vs->features |= VNC_FEATURE_ZRLE_MASK;
            vs->vnc_encoding = enc;
            break;
        case VNC_ENCODING_ZYWRLE:
            vs->features |= VNC_FEATURE_ZYWRLE_MASK;
            vs->vnc_encoding = enc;
            break;
        case VNC_ENCODING_DESKTOPRESIZE:
            vs->features |= VNC_FEATURE_RESIZE_MASK;
            break;
        case VNC_ENCODING_POINTER_TYPE_CHANGE:
            vs->features |= VNC_FEATURE_POINTER_TYPE_CHANGE_MASK;
            break;
        case VNC_ENCODING_RICH_CURSOR:
            vs->features |= VNC_FEATURE_RICH_CURSOR_MASK;
            if (vs->vd->cursor) {
                vnc_cursor_define(vs);
            }
            break;
        case VNC_ENCODING_EXT_KEY_EVENT:
            send_ext_key_event_ack(vs);
            break;
        case VNC_ENCODING_AUDIO:
            send_ext_audio_ack(vs);
            break;
        case VNC_ENCODING_WMVi:
            vs->features |= VNC_FEATURE_WMVI_MASK;
            break;
        case VNC_ENCODING_LED_STATE:
            vs->features |= VNC_FEATURE_LED_STATE_MASK;
            break;
        case VNC_ENCODING_COMPRESSLEVEL0 ... VNC_ENCODING_COMPRESSLEVEL0 + 9:
            vs->tight.compression = (enc & 0x0F);
            break;
        case VNC_ENCODING_QUALITYLEVEL0 ... VNC_ENCODING_QUALITYLEVEL0 + 9:
            if (vs->vd->lossy) {
                vs->tight.quality = (enc & 0x0F);
            }
            break;
        default:
            VNC_DEBUG("Unknown encoding: %d (0x%.8x): %d\n", i, enc, enc);
            break;
        }
    }
    vnc_desktop_resize(vs);
    check_pointer_type_change(&vs->mouse_mode_notifier, NULL);
    vnc_led_state_change(vs);
}