int zmq::curve_client_t::encode (msg_t *msg_)
{
    zmq_assert (state == connected);

    uint8_t flags = 0;
    if (msg_->flags () & msg_t::more)
        flags |= 0x01;

    uint8_t message_nonce [crypto_box_NONCEBYTES];
    memcpy (message_nonce, "CurveZMQMESSAGEC", 16);
    memcpy (message_nonce + 16, &cn_nonce, 8);

    const size_t mlen = crypto_box_ZEROBYTES + 1 + msg_->size ();

    uint8_t *message_plaintext = static_cast <uint8_t *> (malloc (mlen));
    alloc_assert (message_plaintext);

    memset (message_plaintext, 0, crypto_box_ZEROBYTES);
    message_plaintext [crypto_box_ZEROBYTES] = flags;
    memcpy (message_plaintext + crypto_box_ZEROBYTES + 1,
            msg_->data (), msg_->size ());

    uint8_t *message_box = static_cast <uint8_t *> (malloc (mlen));
    alloc_assert (message_box);

    int rc = crypto_box_afternm (message_box, message_plaintext,
                                 mlen, message_nonce, cn_precom);
    zmq_assert (rc == 0);

    rc = msg_->close ();
    zmq_assert (rc == 0);

    rc = msg_->init_size (16 + mlen - crypto_box_BOXZEROBYTES);
    zmq_assert (rc == 0);

    uint8_t *message = static_cast <uint8_t *> (msg_->data ());

    memcpy (message, "\x07MESSAGE", 8);
    memcpy (message + 8, &cn_nonce, 8);
    memcpy (message + 16, message_box + crypto_box_BOXZEROBYTES,
            mlen - crypto_box_BOXZEROBYTES);

    free (message_plaintext);
    free (message_box);

    cn_nonce++;

    return 0;
}