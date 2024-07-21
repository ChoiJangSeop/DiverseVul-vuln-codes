z2restore(i_ctx_t *i_ctx_p)
{
    alloc_save_t *asave;
    bool saveLockSafety = gs_currentdevice_inline(igs)->LockSafetyParams;
    int code = restore_check_save(i_ctx_p, &asave);

    if (code < 0) return code;

    while (gs_gstate_saved(gs_gstate_saved(igs))) {
        if (restore_page_device(igs, gs_gstate_saved(igs)))
            return push_callout(i_ctx_p, "%restore1pagedevice");
        gs_grestore(igs);
    }
    if (restore_page_device(igs, gs_gstate_saved(igs)))
        return push_callout(i_ctx_p, "%restorepagedevice");

    code = dorestore(i_ctx_p, asave);

    if (code < 0) {
        /* An error here is basically fatal, but....
           restore_page_device() has to set LockSafetyParams false so it can
           configure the restored device correctly - in normal operation, that
           gets reset by that configuration. If we hit an error, though, that
           may not happen -  at least ensure we keep the setting through the
           error.
         */
        gs_currentdevice_inline(igs)->LockSafetyParams = saveLockSafety;
    }
    return code;
}