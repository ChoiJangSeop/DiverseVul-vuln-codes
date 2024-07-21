am_cache_entry_t *am_get_request_session_by_nameid(request_rec *r, char *nameid)
{
    return am_cache_lock(r->server, AM_CACHE_NAMEID, nameid);
}