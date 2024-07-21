int zmq::curve_client_t::process_ready (
        const uint8_t *msg_data, size_t msg_size)
{
    if (msg_size < 30) {
        errno = EPROTO;
        return -1;
    }

    const size_t clen = (msg_size - 14) + crypto_box_BOXZEROBYTES;

    uint8_t ready_nonce [crypto_box_NONCEBYTES];
    uint8_t ready_plaintext [crypto_box_ZEROBYTES + 256];
    uint8_t ready_box [crypto_box_BOXZEROBYTES + 16 + 256];

    memset (ready_box, 0, crypto_box_BOXZEROBYTES);
    memcpy (ready_box + crypto_box_BOXZEROBYTES,
            msg_data + 14, clen - crypto_box_BOXZEROBYTES);

    memcpy (ready_nonce, "CurveZMQREADY---", 16);
    memcpy (ready_nonce + 16, msg_data + 6, 8);

    int rc = crypto_box_open_afternm (ready_plaintext, ready_box,
                                      clen, ready_nonce, cn_precom);

    if (rc != 0) {
        errno = EPROTO;
        return -1;
    }

    rc = parse_metadata (ready_plaintext + crypto_box_ZEROBYTES,
                         clen - crypto_box_ZEROBYTES);
    if (rc == 0)
        state = connected;

    return rc;
}