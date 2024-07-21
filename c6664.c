static void h2_session_ev_no_io(h2_session *session, int arg, const char *msg)
{
    switch (session->state) {
        case H2_SESSION_ST_BUSY:
            /* Nothing to READ, nothing to WRITE on the master connection.
             * Possible causes:
             * - we wait for the client to send us sth
             * - we wait for started tasks to produce output
             * - we have finished all streams and the client has sent GO_AWAY
             */
            ap_log_cerror(APLOG_MARK, APLOG_TRACE2, 0, session->c,
                          H2_SSSN_MSG(session, "NO_IO event, %d streams open"), 
                          session->open_streams);
            h2_conn_io_flush(&session->io);
            if (session->open_streams > 0) {
                if (h2_mplx_awaits_data(session->mplx)) {
                    /* waiting for at least one stream to produce data */
                    transit(session, "no io", H2_SESSION_ST_WAIT);
                }
                else {
                    /* we have streams open, and all are submitted and none
                     * is suspended. The only thing keeping us from WRITEing
                     * more must be the flow control.
                     * This means we only wait for WINDOW_UPDATE from the 
                     * client and can block on READ. */
                    transit(session, "no io (flow wait)", H2_SESSION_ST_IDLE);
                    session->idle_until = apr_time_now() + session->s->timeout;
                    session->keep_sync_until = session->idle_until;
                    /* Make sure we have flushed all previously written output
                     * so that the client will react. */
                    if (h2_conn_io_flush(&session->io) != APR_SUCCESS) {
                        dispatch_event(session, H2_SESSION_EV_CONN_ERROR, 0, NULL);
                        return;
                    }
                }
            }
            else if (session->local.accepting) {
                /* When we have no streams, but accept new, switch to idle */
                apr_time_t now = apr_time_now();
                transit(session, "no io (keepalive)", H2_SESSION_ST_IDLE);
                session->idle_until = (session->remote.emitted_count? 
                                       session->s->keep_alive_timeout : 
                                       session->s->timeout) + now;
                session->keep_sync_until = now + apr_time_from_sec(1);
            }
            else {
                /* We are no longer accepting new streams and there are
                 * none left. Time to leave. */
                h2_session_shutdown(session, arg, msg, 0);
                transit(session, "no io", H2_SESSION_ST_DONE);
            }
            break;
        default:
            /* nop */
            break;
    }
}