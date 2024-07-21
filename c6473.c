void h2_beam_abort(h2_bucket_beam *beam)
{
    h2_beam_lock bl;
    
    if (beam && enter_yellow(beam, &bl) == APR_SUCCESS) {
        if (!beam->aborted) {
            beam->aborted = 1;
            r_purge_sent(beam);
            h2_blist_cleanup(&beam->send_list);
            report_consumption(beam, &bl);
        }
        apr_thread_cond_broadcast(beam->change);
        leave_yellow(beam, &bl);
    }
}