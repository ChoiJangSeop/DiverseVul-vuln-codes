z2setgstate(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    check_stype(*op, st_igstate_obj);
    if (!restore_page_device(igs, igstate_ptr(op)))
        return zsetgstate(i_ctx_p);
    return push_callout(i_ctx_p, "%setgstatepagedevice");
}