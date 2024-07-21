int zmq::curve_server_t::process_hello (msg_t *msg_)
{
    if (msg_->size () != 200) {
        //  Temporary support for security debugging
        puts ("CURVE I: client HELLO is not correct size");
        errno = EPROTO;
        return -1;
    }

    const uint8_t * const hello = static_cast <uint8_t *> (msg_->data ());
    if (memcmp (hello, "\x05HELLO", 6)) {
        //  Temporary support for security debugging
        puts ("CURVE I: client HELLO has invalid command name");
        errno = EPROTO;
        return -1;
    }

    const uint8_t major = hello [6];
    const uint8_t minor = hello [7];

    if (major != 1 || minor != 0) {
        //  Temporary support for security debugging
        puts ("CURVE I: client HELLO has unknown version number");
        errno = EPROTO;
        return -1;
    }

    //  Save client's short-term public key (C')
    memcpy (cn_client, hello + 80, 32);

    uint8_t hello_nonce [crypto_box_NONCEBYTES];
    uint8_t hello_plaintext [crypto_box_ZEROBYTES + 64];
    uint8_t hello_box [crypto_box_BOXZEROBYTES + 80];

    memcpy (hello_nonce, "CurveZMQHELLO---", 16);
    memcpy (hello_nonce + 16, hello + 112, 8);

    memset (hello_box, 0, crypto_box_BOXZEROBYTES);
    memcpy (hello_box + crypto_box_BOXZEROBYTES, hello + 120, 80);

    //  Open Box [64 * %x0](C'->S)
    int rc = crypto_box_open (hello_plaintext, hello_box,
                              sizeof hello_box,
                              hello_nonce, cn_client, secret_key);
    if (rc != 0) {
        //  Temporary support for security debugging
        puts ("CURVE I: cannot open client HELLO -- wrong server key?");
        errno = EPROTO;
        return -1;
    }

    state = send_welcome;
    return rc;
}