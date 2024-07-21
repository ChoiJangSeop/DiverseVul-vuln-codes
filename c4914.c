am_cache_entry_t *am_get_request_session(request_rec *r)
{
    const char *session_id;

    /* Get session id from cookie. */
    session_id = am_cookie_get(r);
    if(session_id == NULL) {
        /* Cookie is unset - we don't have a session. */
        return NULL;
    }

    return am_cache_lock(r->server, AM_CACHE_SESSION, session_id);
}