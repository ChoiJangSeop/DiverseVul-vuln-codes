int zmq::curve_server_t::process_initiate (msg_t *msg_)
{
    if (msg_->size () < 257) {
        //  Temporary support for security debugging
        puts ("CURVE I: client INITIATE is not correct size");
        errno = EPROTO;
        return -1;
    }

    const uint8_t *initiate = static_cast <uint8_t *> (msg_->data ());
    if (memcmp (initiate, "\x08INITIATE", 9)) {
        //  Temporary support for security debugging
        puts ("CURVE I: client INITIATE has invalid command name");
        errno = EPROTO;
        return -1;
    }

    uint8_t cookie_nonce [crypto_secretbox_NONCEBYTES];
    uint8_t cookie_plaintext [crypto_secretbox_ZEROBYTES + 64];
    uint8_t cookie_box [crypto_secretbox_BOXZEROBYTES + 80];

    //  Open Box [C' + s'](t)
    memset (cookie_box, 0, crypto_secretbox_BOXZEROBYTES);
    memcpy (cookie_box + crypto_secretbox_BOXZEROBYTES, initiate + 25, 80);

    memcpy (cookie_nonce, "COOKIE--", 8);
    memcpy (cookie_nonce + 8, initiate + 9, 16);

    int rc = crypto_secretbox_open (cookie_plaintext, cookie_box,
                                    sizeof cookie_box,
                                    cookie_nonce, cookie_key);
    if (rc != 0) {
        //  Temporary support for security debugging
        puts ("CURVE I: cannot open client INITIATE cookie");
        errno = EPROTO;
        return -1;
    }

    //  Check cookie plain text is as expected [C' + s']
    if (memcmp (cookie_plaintext + crypto_secretbox_ZEROBYTES, cn_client, 32)
    ||  memcmp (cookie_plaintext + crypto_secretbox_ZEROBYTES + 32, cn_secret, 32)) {
        //  Temporary support for security debugging
        puts ("CURVE I: client INITIATE cookie is not valid");
        errno = EPROTO;
        return -1;
    }

    const size_t clen = (msg_->size () - 113) + crypto_box_BOXZEROBYTES;

    uint8_t initiate_nonce [crypto_box_NONCEBYTES];
    uint8_t initiate_plaintext [crypto_box_ZEROBYTES + 128 + 256];
    uint8_t initiate_box [crypto_box_BOXZEROBYTES + 144 + 256];

    //  Open Box [C + vouch + metadata](C'->S')
    memset (initiate_box, 0, crypto_box_BOXZEROBYTES);
    memcpy (initiate_box + crypto_box_BOXZEROBYTES,
            initiate + 113, clen - crypto_box_BOXZEROBYTES);

    memcpy (initiate_nonce, "CurveZMQINITIATE", 16);
    memcpy (initiate_nonce + 16, initiate + 105, 8);

    rc = crypto_box_open (initiate_plaintext, initiate_box,
                          clen, initiate_nonce, cn_client, cn_secret);
    if (rc != 0) {
        //  Temporary support for security debugging
        puts ("CURVE I: cannot open client INITIATE");
        errno = EPROTO;
        return -1;
    }

    const uint8_t *client_key = initiate_plaintext + crypto_box_ZEROBYTES;

    uint8_t vouch_nonce [crypto_box_NONCEBYTES];
    uint8_t vouch_plaintext [crypto_box_ZEROBYTES + 64];
    uint8_t vouch_box [crypto_box_BOXZEROBYTES + 80];

    //  Open Box Box [C',S](C->S') and check contents
    memset (vouch_box, 0, crypto_box_BOXZEROBYTES);
    memcpy (vouch_box + crypto_box_BOXZEROBYTES,
            initiate_plaintext + crypto_box_ZEROBYTES + 48, 80);

    memcpy (vouch_nonce, "VOUCH---", 8);
    memcpy (vouch_nonce + 8,
            initiate_plaintext + crypto_box_ZEROBYTES + 32, 16);

    rc = crypto_box_open (vouch_plaintext, vouch_box,
                          sizeof vouch_box,
                          vouch_nonce, client_key, cn_secret);
    if (rc != 0) {
        //  Temporary support for security debugging
        puts ("CURVE I: cannot open client INITIATE vouch");
        errno = EPROTO;
        return -1;
    }

    //  What we decrypted must be the client's short-term public key
    if (memcmp (vouch_plaintext + crypto_box_ZEROBYTES, cn_client, 32)) {
        //  Temporary support for security debugging
        puts ("CURVE I: invalid handshake from client (public key)");
        errno = EPROTO;
        return -1;
    }

    //  Precompute connection secret from client key
    rc = crypto_box_beforenm (cn_precom, cn_client, cn_secret);
    zmq_assert (rc == 0);

    //  Use ZAP protocol (RFC 27) to authenticate the user.
    rc = session->zap_connect ();
    if (rc == 0) {
        send_zap_request (client_key);
        rc = receive_and_process_zap_reply ();
        if (rc == 0)
            state = status_code == "200"
                ? send_ready
                : send_error;
        else
        if (errno == EAGAIN)
            state = expect_zap_reply;
        else
            return -1;
    }
    else
        state = send_ready;

    return parse_metadata (initiate_plaintext + crypto_box_ZEROBYTES + 128,
                           clen - crypto_box_ZEROBYTES - 128);
}