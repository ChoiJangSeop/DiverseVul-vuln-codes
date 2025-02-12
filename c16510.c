dtls1_process_buffered_records(SSL *s)
    {
    pitem *item;
    
    item = pqueue_peek(s->d1->unprocessed_rcds.q);
    if (item)
        {
        /* Check if epoch is current. */
        if (s->d1->unprocessed_rcds.epoch != s->d1->r_epoch)
            return(1);  /* Nothing to do. */
        
        /* Process all the records. */
        while (pqueue_peek(s->d1->unprocessed_rcds.q))
            {
            dtls1_get_unprocessed_record(s);
            if ( ! dtls1_process_record(s))
                return(0);
            dtls1_buffer_record(s, &(s->d1->processed_rcds), 
                s->s3->rrec.seq_num);
            }
        }

    /* sync epoch numbers once all the unprocessed records 
     * have been processed */
    s->d1->processed_rcds.epoch = s->d1->r_epoch;
    s->d1->unprocessed_rcds.epoch = s->d1->r_epoch + 1;

    return(1);
    }