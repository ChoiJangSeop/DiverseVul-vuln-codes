int zmq::curve_client_t::produce_initiate (msg_t *msg_)
{
    uint8_t vouch_nonce [crypto_box_NONCEBYTES];
    uint8_t vouch_plaintext [crypto_box_ZEROBYTES + 64];
    uint8_t vouch_box [crypto_box_BOXZEROBYTES + 80];

    //  Create vouch = Box [C',S](C->S')
    memset (vouch_plaintext, 0, crypto_box_ZEROBYTES);
    memcpy (vouch_plaintext + crypto_box_ZEROBYTES, cn_public, 32);
    memcpy (vouch_plaintext + crypto_box_ZEROBYTES + 32, server_key, 32);

    memcpy (vouch_nonce, "VOUCH---", 8);
    randombytes (vouch_nonce + 8, 16);

    int rc = crypto_box (vouch_box, vouch_plaintext,
                         sizeof vouch_plaintext,
                         vouch_nonce, cn_server, secret_key);
    zmq_assert (rc == 0);

    //  Assume here that metadata is limited to 256 bytes
    uint8_t initiate_nonce [crypto_box_NONCEBYTES];
    uint8_t initiate_plaintext [crypto_box_ZEROBYTES + 128 + 256];
    uint8_t initiate_box [crypto_box_BOXZEROBYTES + 144 + 256];

    //  Create Box [C + vouch + metadata](C'->S')
    memset (initiate_plaintext, 0, crypto_box_ZEROBYTES);
    memcpy (initiate_plaintext + crypto_box_ZEROBYTES,
            public_key, 32);
    memcpy (initiate_plaintext + crypto_box_ZEROBYTES + 32,
            vouch_nonce + 8, 16);
    memcpy (initiate_plaintext + crypto_box_ZEROBYTES + 48,
            vouch_box + crypto_box_BOXZEROBYTES, 80);

    //  Metadata starts after vouch
    uint8_t *ptr = initiate_plaintext + crypto_box_ZEROBYTES + 128;

    //  Add socket type property
    const char *socket_type = socket_type_string (options.type);
    ptr += add_property (ptr, "Socket-Type", socket_type, strlen (socket_type));

    //  Add identity property
    if (options.type == ZMQ_REQ
    ||  options.type == ZMQ_DEALER
    ||  options.type == ZMQ_ROUTER)
        ptr += add_property (ptr, "Identity", options.identity, options.identity_size);

    const size_t mlen = ptr - initiate_plaintext;

    memcpy (initiate_nonce, "CurveZMQINITIATE", 16);
    memcpy (initiate_nonce + 16, &cn_nonce, 8);

    rc = crypto_box (initiate_box, initiate_plaintext,
                     mlen, initiate_nonce, cn_server, cn_secret);
    zmq_assert (rc == 0);

    rc = msg_->init_size (113 + mlen - crypto_box_BOXZEROBYTES);
    errno_assert (rc == 0);

    uint8_t *initiate = static_cast <uint8_t *> (msg_->data ());

    memcpy (initiate, "\x08INITIATE", 9);
    //  Cookie provided by the server in the WELCOME command
    memcpy (initiate + 9, cn_cookie, 96);
    //  Short nonce, prefixed by "CurveZMQINITIATE"
    memcpy (initiate + 105, &cn_nonce, 8);
    //  Box [C + vouch + metadata](C'->S')
    memcpy (initiate + 113, initiate_box + crypto_box_BOXZEROBYTES,
            mlen - crypto_box_BOXZEROBYTES);
    cn_nonce++;

    return 0;
}