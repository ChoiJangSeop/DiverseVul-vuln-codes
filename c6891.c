z_jpx_decode(i_ctx_t * i_ctx_p)
{
    os_ptr op = osp;
    ref *sop = NULL;
    ref *csname = NULL;
    stream_jpxd_state state;

    /* it's our responsibility to call set_defaults() */
    state.memory = imemory->non_gc_memory;
    if (s_jpxd_template.set_defaults)
      (*s_jpxd_template.set_defaults)((stream_state *)&state);
    if (r_has_type(op, t_dictionary)) {
        check_dict_read(*op);
        if ( dict_find_string(op, "Alpha", &sop) > 0) {
            check_type(*sop, t_boolean);
            if (sop->value.boolval)
                state.alpha = true;
        }
        if ( dict_find_string(op, "ColorSpace", &sop) > 0) {
            /* parse the value */
            if (r_is_array(sop)) {
                /* assume it's the first array element */
                csname =  sop->value.refs;
            } else if (r_has_type(sop,t_name)) {
                /* use the name directly */
                csname = sop;
            } else {
                dmprintf(imemory, "warning: JPX ColorSpace value is an unhandled type!\n");
            }
            if (csname != NULL) {
                ref sref;
                /* get a reference to the name's string value */
                name_string_ref(imemory, csname, &sref);
                /* request raw index values if the colorspace is /Indexed */
                if (!ISTRCMP(&sref, "Indexed"))
                    state.colorspace = gs_jpx_cs_indexed;
                /* tell the filter what output we want for other spaces */
                else if (!ISTRCMP(&sref, "DeviceGray"))
                    state.colorspace = gs_jpx_cs_gray;
                else if (!ISTRCMP(&sref, "DeviceRGB"))
                    state.colorspace = gs_jpx_cs_rgb;
                else if (!ISTRCMP(&sref, "DeviceCMYK"))
                    state.colorspace = gs_jpx_cs_cmyk;
                else if (!ISTRCMP(&sref, "ICCBased")) {
                    /* The second array element should be the profile's
                       stream dict */
                    ref *csdict = sop->value.refs + 1;
                    ref *nref;
                    ref altname;
                    if (r_is_array(sop) && (r_size(sop) > 1) &&
                      r_has_type(csdict, t_dictionary)) {
                        check_dict_read(*csdict);
                        /* try to look up the alternate space */
                        if (dict_find_string(csdict, "Alternate", &nref) > 0) {
                          name_string_ref(imemory, csname, &altname);
                          if (!ISTRCMP(&altname, "DeviceGray"))
                            state.colorspace = gs_jpx_cs_gray;
                          else if (!ISTRCMP(&altname, "DeviceRGB"))
                            state.colorspace = gs_jpx_cs_rgb;
                          else if (!ISTRCMP(&altname, "DeviceCMYK"))
                            state.colorspace = gs_jpx_cs_cmyk;
                        }
                        /* else guess based on the number of components */
                        if (state.colorspace == gs_jpx_cs_unset &&
                                dict_find_string(csdict, "N", &nref) > 0) {
                          if_debug1m('w', imemory, "[w] JPX image has an external %"PRIpsint
                                     " channel colorspace\n", nref->value.intval);
                          switch (nref->value.intval) {
                            case 1: state.colorspace = gs_jpx_cs_gray;
                                break;
                            case 3: state.colorspace = gs_jpx_cs_rgb;
                                break;
                            case 4: state.colorspace = gs_jpx_cs_cmyk;
                                break;
                          }
                        }
                    }
                }
            } else {
                if_debug0m('w', imemory, "[w] Couldn't read JPX ColorSpace key!\n");
            }
        }
    }

    /* we pass npop=0, since we've no arguments left to consume */
    /* we pass 0 instead of the usual rspace(sop) which will allocate storage
       for filter state from the same memory pool as the stream it's coding.
       this causes no trouble because we maintain no pointers */
    return filter_read(i_ctx_p, 0, &s_jpxd_template,
                       (stream_state *) & state, 0);
}