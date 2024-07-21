z2grestoreall(i_ctx_t *i_ctx_p)
{
    for (;;) {
        if (!restore_page_device(igs, gs_gstate_saved(igs))) {
            bool done = !gs_gstate_saved(gs_gstate_saved(igs));

            gs_grestore(igs);
            if (done)
                break;
        } else
            return push_callout(i_ctx_p, "%grestoreallpagedevice");
    }
    return 0;
}