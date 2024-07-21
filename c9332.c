static h2_push_diary_entry *move_to_last(h2_push_diary *diary, apr_size_t idx)
{
    h2_push_diary_entry *entries = (h2_push_diary_entry*)diary->entries->elts;
    h2_push_diary_entry e;
    apr_size_t lastidx = (apr_size_t)diary->entries->nelts;
    
    /* move entry[idx] to the end */
    if (idx+1 < lastidx) {
        e =  entries[idx];
        memmove(entries+idx, entries+idx+1, sizeof(e) * (lastidx - idx));
        entries[lastidx] = e;
    }
    return &entries[lastidx];
}