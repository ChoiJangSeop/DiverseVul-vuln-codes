zmq::curve_client_t::curve_client_t (const options_t &options_) :
    mechanism_t (options_),
    state (send_hello),
    sync()
{
    memcpy (public_key, options_.curve_public_key, crypto_box_PUBLICKEYBYTES);
    memcpy (secret_key, options_.curve_secret_key, crypto_box_SECRETKEYBYTES);
    memcpy (server_key, options_.curve_server_key, crypto_box_PUBLICKEYBYTES);
    scoped_lock_t lock (sync);
#if defined(HAVE_TWEETNACL)
    // allow opening of /dev/urandom
    unsigned char tmpbytes[4];
    randombytes(tmpbytes, 4);
#else
    // todo check return code
    sodium_init();
#endif

    //  Generate short-term key pair
    const int rc = crypto_box_keypair (cn_public, cn_secret);
    zmq_assert (rc == 0);
}