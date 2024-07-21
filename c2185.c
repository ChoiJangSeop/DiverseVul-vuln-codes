int zmq::curve_client_t::produce_hello (msg_t *msg_)
{
    uint8_t hello_nonce [crypto_box_NONCEBYTES];
    uint8_t hello_plaintext [crypto_box_ZEROBYTES + 64];
    uint8_t hello_box [crypto_box_BOXZEROBYTES + 80];

    //  Prepare the full nonce
    memcpy (hello_nonce, "CurveZMQHELLO---", 16);
    memcpy (hello_nonce + 16, &cn_nonce, 8);

    //  Create Box [64 * %x0](C'->S)
    memset (hello_plaintext, 0, sizeof hello_plaintext);

    int rc = crypto_box (hello_box, hello_plaintext,
                         sizeof hello_plaintext,
                         hello_nonce, server_key, cn_secret);
    zmq_assert (rc == 0);

    rc = msg_->init_size (200);
    errno_assert (rc == 0);
    uint8_t *hello = static_cast <uint8_t *> (msg_->data ());

    memcpy (hello, "\x05HELLO", 6);
    //  CurveZMQ major and minor version numbers
    memcpy (hello + 6, "\1\0", 2);
    //  Anti-amplification padding
    memset (hello + 8, 0, 72);
    //  Client public connection key
    memcpy (hello + 80, cn_public, crypto_box_PUBLICKEYBYTES);
    //  Short nonce, prefixed by "CurveZMQHELLO---"
    memcpy (hello + 112, hello_nonce + 16, 8);
    //  Signature, Box [64 * %x0](C'->S)
    memcpy (hello + 120, hello_box + crypto_box_BOXZEROBYTES, 80);

    cn_nonce++;

    return 0;
}