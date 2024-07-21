static int ssl_parse_record_header( mbedtls_ssl_context const *ssl,
                                    unsigned char *buf,
                                    size_t len,
                                    mbedtls_record *rec )
{
    int major_ver, minor_ver;

    size_t const rec_hdr_type_offset    = 0;
    size_t const rec_hdr_type_len       = 1;

    size_t const rec_hdr_version_offset = rec_hdr_type_offset +
                                          rec_hdr_type_len;
    size_t const rec_hdr_version_len    = 2;

    size_t const rec_hdr_ctr_len        = 8;
#if defined(MBEDTLS_SSL_PROTO_DTLS)
    uint32_t     rec_epoch;
    size_t const rec_hdr_ctr_offset     = rec_hdr_version_offset +
                                          rec_hdr_version_len;

#if defined(MBEDTLS_SSL_DTLS_CONNECTION_ID)
    size_t const rec_hdr_cid_offset     = rec_hdr_ctr_offset +
                                          rec_hdr_ctr_len;
    size_t       rec_hdr_cid_len        = 0;
#endif /* MBEDTLS_SSL_DTLS_CONNECTION_ID */
#endif /* MBEDTLS_SSL_PROTO_DTLS */

    size_t       rec_hdr_len_offset; /* To be determined */
    size_t const rec_hdr_len_len    = 2;

    /*
     * Check minimum lengths for record header.
     */

#if defined(MBEDTLS_SSL_PROTO_DTLS)
    if( ssl->conf->transport == MBEDTLS_SSL_TRANSPORT_DATAGRAM )
    {
        rec_hdr_len_offset = rec_hdr_ctr_offset + rec_hdr_ctr_len;
    }
    else
#endif /* MBEDTLS_SSL_PROTO_DTLS */
    {
        rec_hdr_len_offset = rec_hdr_version_offset + rec_hdr_version_len;
    }

    if( len < rec_hdr_len_offset + rec_hdr_len_len )
    {
        MBEDTLS_SSL_DEBUG_MSG( 1, ( "datagram of length %u too small to hold DTLS record header of length %u",
                 (unsigned) len,
                 (unsigned)( rec_hdr_len_len + rec_hdr_len_len ) ) );
        return( MBEDTLS_ERR_SSL_INVALID_RECORD );
    }

    /*
     * Parse and validate record content type
     */

    rec->type = buf[ rec_hdr_type_offset ];

    /* Check record content type */
#if defined(MBEDTLS_SSL_DTLS_CONNECTION_ID)
    rec->cid_len = 0;

    if( ssl->conf->transport == MBEDTLS_SSL_TRANSPORT_DATAGRAM &&
        ssl->conf->cid_len != 0                                &&
        rec->type == MBEDTLS_SSL_MSG_CID )
    {
        /* Shift pointers to account for record header including CID
         * struct {
         *   ContentType special_type = tls12_cid;
         *   ProtocolVersion version;
         *   uint16 epoch;
         *   uint48 sequence_number;
         *   opaque cid[cid_length]; // Additional field compared to
         *                           // default DTLS record format
         *   uint16 length;
         *   opaque enc_content[DTLSCiphertext.length];
         * } DTLSCiphertext;
         */

        /* So far, we only support static CID lengths
         * fixed in the configuration. */
        rec_hdr_cid_len = ssl->conf->cid_len;
        rec_hdr_len_offset += rec_hdr_cid_len;

        if( len < rec_hdr_len_offset + rec_hdr_len_len )
        {
            MBEDTLS_SSL_DEBUG_MSG( 1, ( "datagram of length %u too small to hold DTLS record header including CID, length %u",
                (unsigned) len,
                (unsigned)( rec_hdr_len_offset + rec_hdr_len_len ) ) );
            return( MBEDTLS_ERR_SSL_INVALID_RECORD );
        }

        /* configured CID len is guaranteed at most 255, see
         * MBEDTLS_SSL_CID_OUT_LEN_MAX in check_config.h */
        rec->cid_len = (uint8_t) rec_hdr_cid_len;
        memcpy( rec->cid, buf + rec_hdr_cid_offset, rec_hdr_cid_len );
    }
    else
#endif /* MBEDTLS_SSL_DTLS_CONNECTION_ID */
    {
        if( ssl_check_record_type( rec->type ) )
        {
            MBEDTLS_SSL_DEBUG_MSG( 1, ( "unknown record type %u",
                                        (unsigned) rec->type ) );
            return( MBEDTLS_ERR_SSL_INVALID_RECORD );
        }
    }

    /*
     * Parse and validate record version
     */

    rec->ver[0] = buf[ rec_hdr_version_offset + 0 ];
    rec->ver[1] = buf[ rec_hdr_version_offset + 1 ];
    mbedtls_ssl_read_version( &major_ver, &minor_ver,
                              ssl->conf->transport,
                              &rec->ver[0] );

    if( major_ver != ssl->major_ver )
    {
        MBEDTLS_SSL_DEBUG_MSG( 1, ( "major version mismatch" ) );
        return( MBEDTLS_ERR_SSL_INVALID_RECORD );
    }

    if( minor_ver > ssl->conf->max_minor_ver )
    {
        MBEDTLS_SSL_DEBUG_MSG( 1, ( "minor version mismatch" ) );
        return( MBEDTLS_ERR_SSL_INVALID_RECORD );
    }

    /*
     * Parse/Copy record sequence number.
     */

#if defined(MBEDTLS_SSL_PROTO_DTLS)
    if( ssl->conf->transport == MBEDTLS_SSL_TRANSPORT_DATAGRAM )
    {
        /* Copy explicit record sequence number from input buffer. */
        memcpy( &rec->ctr[0], buf + rec_hdr_ctr_offset,
                rec_hdr_ctr_len );
    }
    else
#endif /* MBEDTLS_SSL_PROTO_DTLS */
    {
        /* Copy implicit record sequence number from SSL context structure. */
        memcpy( &rec->ctr[0], ssl->in_ctr, rec_hdr_ctr_len );
    }

    /*
     * Parse record length.
     */

    rec->data_offset = rec_hdr_len_offset + rec_hdr_len_len;
    rec->data_len    = ( (size_t) buf[ rec_hdr_len_offset + 0 ] << 8 ) |
                       ( (size_t) buf[ rec_hdr_len_offset + 1 ] << 0 );
    MBEDTLS_SSL_DEBUG_BUF( 4, "input record header", buf, rec->data_offset );

    MBEDTLS_SSL_DEBUG_MSG( 3, ( "input record: msgtype = %u, "
                                "version = [%d:%d], msglen = %" MBEDTLS_PRINTF_SIZET,
                                rec->type,
                                major_ver, minor_ver, rec->data_len ) );

    rec->buf     = buf;
    rec->buf_len = rec->data_offset + rec->data_len;

    if( rec->data_len == 0 )
        return( MBEDTLS_ERR_SSL_INVALID_RECORD );

    /*
     * DTLS-related tests.
     * Check epoch before checking length constraint because
     * the latter varies with the epoch. E.g., if a ChangeCipherSpec
     * message gets duplicated before the corresponding Finished message,
     * the second ChangeCipherSpec should be discarded because it belongs
     * to an old epoch, but not because its length is shorter than
     * the minimum record length for packets using the new record transform.
     * Note that these two kinds of failures are handled differently,
     * as an unexpected record is silently skipped but an invalid
     * record leads to the entire datagram being dropped.
     */
#if defined(MBEDTLS_SSL_PROTO_DTLS)
    if( ssl->conf->transport == MBEDTLS_SSL_TRANSPORT_DATAGRAM )
    {
        rec_epoch = ( rec->ctr[0] << 8 ) | rec->ctr[1];

        /* Check that the datagram is large enough to contain a record
         * of the advertised length. */
        if( len < rec->data_offset + rec->data_len )
        {
            MBEDTLS_SSL_DEBUG_MSG( 1, ( "Datagram of length %u too small to contain record of advertised length %u.",
                             (unsigned) len,
                             (unsigned)( rec->data_offset + rec->data_len ) ) );
            return( MBEDTLS_ERR_SSL_INVALID_RECORD );
        }

        /* Records from other, non-matching epochs are silently discarded.
         * (The case of same-port Client reconnects must be considered in
         *  the caller). */
        if( rec_epoch != ssl->in_epoch )
        {
            MBEDTLS_SSL_DEBUG_MSG( 1, ( "record from another epoch: "
                                        "expected %u, received %lu",
                                        ssl->in_epoch, (unsigned long) rec_epoch ) );

            /* Records from the next epoch are considered for buffering
             * (concretely: early Finished messages). */
            if( rec_epoch == (unsigned) ssl->in_epoch + 1 )
            {
                MBEDTLS_SSL_DEBUG_MSG( 2, ( "Consider record for buffering" ) );
                return( MBEDTLS_ERR_SSL_EARLY_MESSAGE );
            }

            return( MBEDTLS_ERR_SSL_UNEXPECTED_RECORD );
        }
#if defined(MBEDTLS_SSL_DTLS_ANTI_REPLAY)
        /* For records from the correct epoch, check whether their
         * sequence number has been seen before. */
        else if( mbedtls_ssl_dtls_record_replay_check( (mbedtls_ssl_context *) ssl,
            &rec->ctr[0] ) != 0 )
        {
            MBEDTLS_SSL_DEBUG_MSG( 1, ( "replayed record" ) );
            return( MBEDTLS_ERR_SSL_UNEXPECTED_RECORD );
        }
#endif
    }
#endif /* MBEDTLS_SSL_PROTO_DTLS */

    return( 0 );
}