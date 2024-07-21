z2restore(i_ctx_t *i_ctx_p)
{
    while (gs_gstate_saved(gs_gstate_saved(igs))) {
        if (restore_page_device(igs, gs_gstate_saved(igs)))
            return push_callout(i_ctx_p, "%restore1pagedevice");
        gs_grestore(igs);
    }
    if (restore_page_device(igs, gs_gstate_saved(igs)))
        return push_callout(i_ctx_p, "%restorepagedevice");
    return zrestore(i_ctx_p);
}