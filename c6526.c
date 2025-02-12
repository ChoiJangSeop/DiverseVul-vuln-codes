ref_param_read_signal_error(gs_param_list * plist, gs_param_name pkey, int code)
{
    iparam_list *const iplist = (iparam_list *) plist;
    iparam_loc loc;

    ref_param_read(iplist, pkey, &loc, -1);	/* can't fail */
    *loc.presult = code;
    switch (ref_param_read_get_policy(plist, pkey)) {
        case gs_param_policy_ignore:
            return 0;
        case gs_param_policy_consult_user:
            return_error(gs_error_configurationerror);
        default:
            return code;
    }
}