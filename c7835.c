static void transit(h2_session *session, const char *action, h2_session_state nstate)
{
    apr_time_t timeout;
    int ostate, loglvl;
    const char *s;
    
    if (session->state != nstate) {
        ostate = session->state;
        session->state = nstate;
        
        loglvl = APLOG_DEBUG;
        if ((ostate == H2_SESSION_ST_BUSY && nstate == H2_SESSION_ST_WAIT)
            || (ostate == H2_SESSION_ST_WAIT && nstate == H2_SESSION_ST_BUSY)){
            loglvl = APLOG_TRACE1;
        }
        ap_log_cerror(APLOG_MARK, loglvl, 0, session->c, 
                      H2_SSSN_LOG(APLOGNO(03078), session, 
                      "transit [%s] -- %s --> [%s]"), 
                      h2_session_state_str(ostate), action, 
                      h2_session_state_str(nstate));
        
        switch (session->state) {
            case H2_SESSION_ST_IDLE:
                if (!session->remote.emitted_count) {
                    /* on fresh connections, with async mpm, do not return
                     * to mpm for a second. This gives the first request a better
                     * chance to arrive (und connection leaving IDLE state).
                     * If we return to mpm right away, this connection has the
                     * same chance of being cleaned up by the mpm as connections
                     * that already served requests - not fair. */
                    session->idle_sync_until = apr_time_now() + apr_time_from_sec(1);
                    s = "timeout";
                    timeout = H2MAX(session->s->timeout, session->s->keep_alive_timeout);
                    update_child_status(session, SERVER_BUSY_READ, "idle");
                    ap_log_cerror(APLOG_MARK, APLOG_TRACE1, 0, session->c, 
                                  H2_SSSN_LOG("", session, "enter idle, timeout = %d sec"), 
                                  (int)apr_time_sec(H2MAX(session->s->timeout, session->s->keep_alive_timeout)));
                }
                else if (session->open_streams) {
                    s = "timeout";
                    timeout = session->s->keep_alive_timeout;
                    update_child_status(session, SERVER_BUSY_KEEPALIVE, "idle");
                }
                else {
                    /* normal keepalive setup */
                    s = "keepalive";
                    timeout = session->s->keep_alive_timeout;
                    update_child_status(session, SERVER_BUSY_KEEPALIVE, "idle");
                }
                session->idle_until = apr_time_now() + timeout; 
                ap_log_cerror(APLOG_MARK, APLOG_TRACE1, 0, session->c, 
                              H2_SSSN_LOG("", session, "enter idle, %s = %d sec"), 
                              s, (int)apr_time_sec(timeout));
                break;
            case H2_SESSION_ST_DONE:
                update_child_status(session, SERVER_CLOSING, "done");
                break;
            default:
                /* nop */
                break;
        }
    }
}