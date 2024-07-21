static int dtls1_process_buffered_records(SSL *s)
{
    pitem *item;
    SSL3_BUFFER *rb;

    item = pqueue_peek(s->d1->unprocessed_rcds.q);
    if (item) {
        /* Check if epoch is current. */
        if (s->d1->unprocessed_rcds.epoch != s->d1->r_epoch)
            return (1);         /* Nothing to do. */

        rb = &s->s3->rbuf;

        if (rb->left > 0) {
            /*
             * We've still got data from the current packet to read. There could
             * be a record from the new epoch in it - so don't overwrite it
             * with the unprocessed records yet (we'll do it when we've
             * finished reading the current packet).
             */
            return 1;
        }


        /* Process all the records. */
        while (pqueue_peek(s->d1->unprocessed_rcds.q)) {
            dtls1_get_unprocessed_record(s);
            if (!dtls1_process_record(s))
                return (0);
            if (dtls1_buffer_record(s, &(s->d1->processed_rcds),
                                    s->s3->rrec.seq_num) < 0)
                return -1;
        }
    }

    /*
     * sync epoch numbers once all the unprocessed records have been
     * processed
     */
    s->d1->processed_rcds.epoch = s->d1->r_epoch;
    s->d1->unprocessed_rcds.epoch = s->d1->r_epoch + 1;

    return (1);
}