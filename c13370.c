find_jws(struct tang_keys_info* tki, const char* thp)
{
    if (!tki) {
        return NULL;
    }

    if (thp == NULL) {
        /* Default advertisement. */
        json_auto_t* jws = jwk_sign(tki->m_payload, tki->m_sign);
        if (!jws) {
            return NULL;
        }
        return json_incref(jws);
    }
    return find_by_thp(tki, thp);
}