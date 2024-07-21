static void recv_buffer_cleanup(h2_bucket_beam *beam, h2_beam_lock *bl)
{
    if (beam->recv_buffer && !APR_BRIGADE_EMPTY(beam->recv_buffer)) {
        apr_bucket_brigade *bb = beam->recv_buffer;
        apr_off_t bblen = 0;
        
        beam->recv_buffer = NULL;
        apr_brigade_length(bb, 0, &bblen);
        beam->received_bytes += bblen;
        
        /* need to do this unlocked since bucket destroy might 
         * call this beam again. */
        if (bl) leave_yellow(beam, bl);
        apr_brigade_destroy(bb);
        if (bl) enter_yellow(beam, bl);
        
        if (beam->cons_ev_cb) { 
            beam->cons_ev_cb(beam->cons_ctx, beam);
        }
    }
}