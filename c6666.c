static void h2_session_ev_data_read(h2_session *session, int arg, const char *msg)
{
    switch (session->state) {
        case H2_SESSION_ST_IDLE:
        case H2_SESSION_ST_WAIT:
            transit(session, "data read", H2_SESSION_ST_BUSY);
            break;
        default:
            /* nop */
            break;
    }
}