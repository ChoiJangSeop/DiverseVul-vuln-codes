int _libssh2_transport_read(LIBSSH2_SESSION * session)
{
    int rc;
    struct transportpacket *p = &session->packet;
    int remainbuf;
    int remainpack;
    int numbytes;
    int numdecrypt;
    unsigned char block[MAX_BLOCKSIZE];
    int blocksize;
    int encrypted = 1;
    size_t total_num;

    /* default clear the bit */
    session->socket_block_directions &= ~LIBSSH2_SESSION_BLOCK_INBOUND;

    /*
     * All channels, systems, subsystems, etc eventually make it down here
     * when looking for more incoming data. If a key exchange is going on
     * (LIBSSH2_STATE_EXCHANGING_KEYS bit is set) then the remote end will
     * ONLY send key exchange related traffic. In non-blocking mode, there is
     * a chance to break out of the kex_exchange function with an EAGAIN
     * status, and never come back to it. If LIBSSH2_STATE_EXCHANGING_KEYS is
     * active, then we must redirect to the key exchange. However, if
     * kex_exchange is active (as in it is the one that calls this execution
     * of packet_read, then don't redirect, as that would be an infinite loop!
     */

    if(session->state & LIBSSH2_STATE_EXCHANGING_KEYS &&
        !(session->state & LIBSSH2_STATE_KEX_ACTIVE)) {

        /* Whoever wants a packet won't get anything until the key re-exchange
         * is done!
         */
        _libssh2_debug(session, LIBSSH2_TRACE_TRANS, "Redirecting into the"
                       " key re-exchange from _libssh2_transport_read");
        rc = _libssh2_kex_exchange(session, 1, &session->startup_key_state);
        if(rc)
            return rc;
    }

    /*
     * =============================== NOTE ===============================
     * I know this is very ugly and not a really good use of "goto", but
     * this case statement would be even uglier to do it any other way
     */
    if(session->readPack_state == libssh2_NB_state_jump1) {
        session->readPack_state = libssh2_NB_state_idle;
        encrypted = session->readPack_encrypted;
        goto libssh2_transport_read_point1;
    }

    do {
        if(session->socket_state == LIBSSH2_SOCKET_DISCONNECTED) {
            return LIBSSH2_ERROR_NONE;
        }

        if(session->state & LIBSSH2_STATE_NEWKEYS) {
            blocksize = session->remote.crypt->blocksize;
        }
        else {
            encrypted = 0;      /* not encrypted */
            blocksize = 5;      /* not strictly true, but we can use 5 here to
                                   make the checks below work fine still */
        }

        /* read/use a whole big chunk into a temporary area stored in
           the LIBSSH2_SESSION struct. We will decrypt data from that
           buffer into the packet buffer so this temp one doesn't have
           to be able to keep a whole SSH packet, just be large enough
           so that we can read big chunks from the network layer. */

        /* how much data there is remaining in the buffer to deal with
           before we should read more from the network */
        remainbuf = p->writeidx - p->readidx;

        /* if remainbuf turns negative we have a bad internal error */
        assert(remainbuf >= 0);

        if(remainbuf < blocksize) {
            /* If we have less than a blocksize left, it is too
               little data to deal with, read more */
            ssize_t nread;

            /* move any remainder to the start of the buffer so
               that we can do a full refill */
            if(remainbuf) {
                memmove(p->buf, &p->buf[p->readidx], remainbuf);
                p->readidx = 0;
                p->writeidx = remainbuf;
            }
            else {
                /* nothing to move, just zero the indexes */
                p->readidx = p->writeidx = 0;
            }

            /* now read a big chunk from the network into the temp buffer */
            nread =
                LIBSSH2_RECV(session, &p->buf[remainbuf],
                              PACKETBUFSIZE - remainbuf,
                              LIBSSH2_SOCKET_RECV_FLAGS(session));
            if(nread <= 0) {
                /* check if this is due to EAGAIN and return the special
                   return code if so, error out normally otherwise */
                if((nread < 0) && (nread == -EAGAIN)) {
                    session->socket_block_directions |=
                        LIBSSH2_SESSION_BLOCK_INBOUND;
                    return LIBSSH2_ERROR_EAGAIN;
                }
                _libssh2_debug(session, LIBSSH2_TRACE_SOCKET,
                               "Error recving %d bytes (got %d)",
                               PACKETBUFSIZE - remainbuf, -nread);
                return LIBSSH2_ERROR_SOCKET_RECV;
            }
            _libssh2_debug(session, LIBSSH2_TRACE_SOCKET,
                           "Recved %d/%d bytes to %p+%d", nread,
                           PACKETBUFSIZE - remainbuf, p->buf, remainbuf);

            debugdump(session, "libssh2_transport_read() raw",
                      &p->buf[remainbuf], nread);
            /* advance write pointer */
            p->writeidx += nread;

            /* update remainbuf counter */
            remainbuf = p->writeidx - p->readidx;
        }

        /* how much data to deal with from the buffer */
        numbytes = remainbuf;

        if(!p->total_num) {
            /* No payload package area allocated yet. To know the
               size of this payload, we need to decrypt the first
               blocksize data. */

            if(numbytes < blocksize) {
                /* we can't act on anything less than blocksize, but this
                   check is only done for the initial block since once we have
                   got the start of a block we can in fact deal with fractions
                */
                session->socket_block_directions |=
                    LIBSSH2_SESSION_BLOCK_INBOUND;
                return LIBSSH2_ERROR_EAGAIN;
            }

            if(encrypted) {
                rc = decrypt(session, &p->buf[p->readidx], block, blocksize);
                if(rc != LIBSSH2_ERROR_NONE) {
                    return rc;
                }
                /* save the first 5 bytes of the decrypted package, to be
                   used in the hash calculation later down. */
                memcpy(p->init, block, 5);
            }
            else {
                /* the data is plain, just copy it verbatim to
                   the working block buffer */
                memcpy(block, &p->buf[p->readidx], blocksize);
            }

            /* advance the read pointer */
            p->readidx += blocksize;

            /* we now have the initial blocksize bytes decrypted,
             * and we can extract packet and padding length from it
             */
            p->packet_length = _libssh2_ntohu32(block);
            if(p->packet_length < 1)
                return LIBSSH2_ERROR_DECRYPT;

            p->padding_length = block[4];

            /* total_num is the number of bytes following the initial
               (5 bytes) packet length and padding length fields */
            total_num =
                p->packet_length - 1 +
                (encrypted ? session->remote.mac->mac_len : 0);

            /* RFC4253 section 6.1 Maximum Packet Length says:
             *
             * "All implementations MUST be able to process
             * packets with uncompressed payload length of 32768
             * bytes or less and total packet size of 35000 bytes
             * or less (including length, padding length, payload,
             * padding, and MAC.)."
             */
            if(total_num > LIBSSH2_PACKET_MAXPAYLOAD) {
                return LIBSSH2_ERROR_OUT_OF_BOUNDARY;
            }

            /* Get a packet handle put data into. We get one to
               hold all data, including padding and MAC. */
            p->payload = LIBSSH2_ALLOC(session, total_num);
            if(!p->payload) {
                return LIBSSH2_ERROR_ALLOC;
            }
            p->total_num = total_num;
            /* init write pointer to start of payload buffer */
            p->wptr = p->payload;

            if(blocksize > 5) {
                /* copy the data from index 5 to the end of
                   the blocksize from the temporary buffer to
                   the start of the decrypted buffer */
                memcpy(p->wptr, &block[5], blocksize - 5);
                p->wptr += blocksize - 5;       /* advance write pointer */
            }

            /* init the data_num field to the number of bytes of
               the package read so far */
            p->data_num = p->wptr - p->payload;

            /* we already dealt with a blocksize worth of data */
            numbytes -= blocksize;
        }

        /* how much there is left to add to the current payload
           package */
        remainpack = p->total_num - p->data_num;

        if(numbytes > remainpack) {
            /* if we have more data in the buffer than what is going into this
               particular packet, we limit this round to this packet only */
            numbytes = remainpack;
        }

        if(encrypted) {
            /* At the end of the incoming stream, there is a MAC,
               and we don't want to decrypt that since we need it
               "raw". We MUST however decrypt the padding data
               since it is used for the hash later on. */
            int skip = session->remote.mac->mac_len;

            /* if what we have plus numbytes is bigger than the
               total minus the skip margin, we should lower the
               amount to decrypt even more */
            if((p->data_num + numbytes) > (p->total_num - skip)) {
                numdecrypt = (p->total_num - skip) - p->data_num;
            }
            else {
                int frac;
                numdecrypt = numbytes;
                frac = numdecrypt % blocksize;
                if(frac) {
                    /* not an aligned amount of blocks,
                       align it */
                    numdecrypt -= frac;
                    /* and make it no unencrypted data
                       after it */
                    numbytes = 0;
                }
            }
        }
        else {
            /* unencrypted data should not be decrypted at all */
            numdecrypt = 0;
        }

        /* if there are bytes to decrypt, do that */
        if(numdecrypt > 0) {
            /* now decrypt the lot */
            rc = decrypt(session, &p->buf[p->readidx], p->wptr, numdecrypt);
            if(rc != LIBSSH2_ERROR_NONE) {
                p->total_num = 0;   /* no packet buffer available */
                return rc;
            }

            /* advance the read pointer */
            p->readidx += numdecrypt;
            /* advance write pointer */
            p->wptr += numdecrypt;
            /* increase data_num */
            p->data_num += numdecrypt;

            /* bytes left to take care of without decryption */
            numbytes -= numdecrypt;
        }

        /* if there are bytes to copy that aren't decrypted, simply
           copy them as-is to the target buffer */
        if(numbytes > 0) {
            memcpy(p->wptr, &p->buf[p->readidx], numbytes);

            /* advance the read pointer */
            p->readidx += numbytes;
            /* advance write pointer */
            p->wptr += numbytes;
            /* increase data_num */
            p->data_num += numbytes;
        }

        /* now check how much data there's left to read to finish the
           current packet */
        remainpack = p->total_num - p->data_num;

        if(!remainpack) {
            /* we have a full packet */
          libssh2_transport_read_point1:
            rc = fullpacket(session, encrypted);
            if(rc == LIBSSH2_ERROR_EAGAIN) {

                if(session->packAdd_state != libssh2_NB_state_idle) {
                    /* fullpacket only returns LIBSSH2_ERROR_EAGAIN if
                     * libssh2_packet_add returns LIBSSH2_ERROR_EAGAIN. If that
                     * returns LIBSSH2_ERROR_EAGAIN but the packAdd_state is idle,
                     * then the packet has been added to the brigade, but some
                     * immediate action that was taken based on the packet
                     * type (such as key re-exchange) is not yet complete.
                     * Clear the way for a new packet to be read in.
                     */
                    session->readPack_encrypted = encrypted;
                    session->readPack_state = libssh2_NB_state_jump1;
                }

                return rc;
            }

            p->total_num = 0;   /* no packet buffer available */

            return rc;
        }
    } while(1);                /* loop */

    return LIBSSH2_ERROR_SOCKET_RECV; /* we never reach this point */
}