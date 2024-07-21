int zmq::curve_server_t::produce_ready (msg_t *msg_)
{
    uint8_t ready_nonce [crypto_box_NONCEBYTES];
    uint8_t ready_plaintext [crypto_box_ZEROBYTES + 256];
    uint8_t ready_box [crypto_box_BOXZEROBYTES + 16 + 256];

    //  Create Box [metadata](S'->C')
    memset (ready_plaintext, 0, crypto_box_ZEROBYTES);
    uint8_t *ptr = ready_plaintext + crypto_box_ZEROBYTES;

    //  Add socket type property
    const char *socket_type = socket_type_string (options.type);
    ptr += add_property (ptr, "Socket-Type", socket_type, strlen (socket_type));

    //  Add identity property
    if (options.type == ZMQ_REQ
    ||  options.type == ZMQ_DEALER
    ||  options.type == ZMQ_ROUTER)
        ptr += add_property (ptr, "Identity", options.identity, options.identity_size);

    const size_t mlen = ptr - ready_plaintext;

    memcpy (ready_nonce, "CurveZMQREADY---", 16);
    memcpy (ready_nonce + 16, &cn_nonce, 8);

    int rc = crypto_box_afternm (ready_box, ready_plaintext,
                                 mlen, ready_nonce, cn_precom);
    zmq_assert (rc == 0);

    rc = msg_->init_size (14 + mlen - crypto_box_BOXZEROBYTES);
    errno_assert (rc == 0);

    uint8_t *ready = static_cast <uint8_t *> (msg_->data ());

    memcpy (ready, "\x05READY", 6);
    //  Short nonce, prefixed by "CurveZMQREADY---"
    memcpy (ready + 6, &cn_nonce, 8);
    //  Box [metadata](S'->C')
    memcpy (ready + 14, ready_box + crypto_box_BOXZEROBYTES,
            mlen - crypto_box_BOXZEROBYTES);

    cn_nonce++;

    return 0;
}