static int ssl_check_dtls_clihlo_cookie(
                           mbedtls_ssl_context *ssl,
                           const unsigned char *cli_id, size_t cli_id_len,
                           const unsigned char *in, size_t in_len,
                           unsigned char *obuf, size_t buf_len, size_t *olen )
{
    size_t sid_len, cookie_len;
    unsigned char *p;

    /*
     * Structure of ClientHello with record and handshake headers,
     * and expected values. We don't need to check a lot, more checks will be
     * done when actually parsing the ClientHello - skipping those checks
     * avoids code duplication and does not make cookie forging any easier.
     *
     *  0-0  ContentType type;                  copied, must be handshake
     *  1-2  ProtocolVersion version;           copied
     *  3-4  uint16 epoch;                      copied, must be 0
     *  5-10 uint48 sequence_number;            copied
     * 11-12 uint16 length;                     (ignored)
     *
     * 13-13 HandshakeType msg_type;            (ignored)
     * 14-16 uint24 length;                     (ignored)
     * 17-18 uint16 message_seq;                copied
     * 19-21 uint24 fragment_offset;            copied, must be 0
     * 22-24 uint24 fragment_length;            (ignored)
     *
     * 25-26 ProtocolVersion client_version;    (ignored)
     * 27-58 Random random;                     (ignored)
     * 59-xx SessionID session_id;              1 byte len + sid_len content
     * 60+   opaque cookie<0..2^8-1>;           1 byte len + content
     *       ...
     *
     * Minimum length is 61 bytes.
     */
    MBEDTLS_SSL_DEBUG_MSG( 4, ( "check cookie: in_len=%u",
                                (unsigned) in_len ) );
    MBEDTLS_SSL_DEBUG_BUF( 4, "cli_id", cli_id, cli_id_len );
    if( in_len < 61 )
    {
        MBEDTLS_SSL_DEBUG_MSG( 4, ( "check cookie: record too short" ) );
        return( MBEDTLS_ERR_SSL_BAD_HS_CLIENT_HELLO );
    }
    if( in[0] != MBEDTLS_SSL_MSG_HANDSHAKE ||
        in[3] != 0 || in[4] != 0 ||
        in[19] != 0 || in[20] != 0 || in[21] != 0 )
    {
        MBEDTLS_SSL_DEBUG_MSG( 4, ( "check cookie: not a good ClientHello" ) );
        MBEDTLS_SSL_DEBUG_MSG( 4, ( "    type=%u epoch=%u fragment_offset=%u",
                                    in[0],
                                    (unsigned) in[3] << 8 | in[4],
                                    (unsigned) in[19] << 16 | in[20] << 8 | in[21] ) );
        return( MBEDTLS_ERR_SSL_BAD_HS_CLIENT_HELLO );
    }

    sid_len = in[59];
    if( sid_len > in_len - 61 )
    {
        MBEDTLS_SSL_DEBUG_MSG( 4, ( "check cookie: sid_len=%u > %u",
                                    (unsigned) sid_len,
                                    (unsigned) in_len - 61 ) );
        return( MBEDTLS_ERR_SSL_BAD_HS_CLIENT_HELLO );
    }
    MBEDTLS_SSL_DEBUG_BUF( 4, "sid received from network",
                           in + 60, sid_len );

    cookie_len = in[60 + sid_len];
    if( cookie_len > in_len - 60 ) {
        MBEDTLS_SSL_DEBUG_MSG( 4, ( "check cookie: cookie_len=%u > %u",
                                    (unsigned) cookie_len,
                                    (unsigned) in_len - 60 ) );
        return( MBEDTLS_ERR_SSL_BAD_HS_CLIENT_HELLO );
    }

    MBEDTLS_SSL_DEBUG_BUF( 4, "cookie received from network",
                           in + sid_len + 61, cookie_len );
    if( ssl->conf->f_cookie_check( ssl->conf->p_cookie,
                                   in + sid_len + 61, cookie_len,
                                   cli_id, cli_id_len ) == 0 )
    {
        MBEDTLS_SSL_DEBUG_MSG( 4, ( "check cookie: valid" ) );
        return( 0 );
    }

    /*
     * If we get here, we've got an invalid cookie, let's prepare HVR.
     *
     *  0-0  ContentType type;                  copied
     *  1-2  ProtocolVersion version;           copied
     *  3-4  uint16 epoch;                      copied
     *  5-10 uint48 sequence_number;            copied
     * 11-12 uint16 length;                     olen - 13
     *
     * 13-13 HandshakeType msg_type;            hello_verify_request
     * 14-16 uint24 length;                     olen - 25
     * 17-18 uint16 message_seq;                copied
     * 19-21 uint24 fragment_offset;            copied
     * 22-24 uint24 fragment_length;            olen - 25
     *
     * 25-26 ProtocolVersion server_version;    0xfe 0xff
     * 27-27 opaque cookie<0..2^8-1>;           cookie_len = olen - 27, cookie
     *
     * Minimum length is 28.
     */
    if( buf_len < 28 )
        return( MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL );

    /* Copy most fields and adapt others */
    memcpy( obuf, in, 25 );
    obuf[13] = MBEDTLS_SSL_HS_HELLO_VERIFY_REQUEST;
    obuf[25] = 0xfe;
    obuf[26] = 0xff;

    /* Generate and write actual cookie */
    p = obuf + 28;
    if( ssl->conf->f_cookie_write( ssl->conf->p_cookie,
                                   &p, obuf + buf_len,
                                   cli_id, cli_id_len ) != 0 )
    {
        return( MBEDTLS_ERR_SSL_INTERNAL_ERROR );
    }

    *olen = p - obuf;

    /* Go back and fill length fields */
    obuf[27] = (unsigned char)( *olen - 28 );

    obuf[14] = obuf[22] = MBEDTLS_BYTE_2( *olen - 25 );
    obuf[15] = obuf[23] = MBEDTLS_BYTE_1( *olen - 25 );
    obuf[16] = obuf[24] = MBEDTLS_BYTE_0( *olen - 25 );

    MBEDTLS_PUT_UINT16_BE( *olen - 13, obuf, 11 );

    return( MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED );
}