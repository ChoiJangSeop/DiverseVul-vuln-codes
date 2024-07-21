do_compose_group16(pdf14_buf *tos, pdf14_buf *nos, pdf14_buf *maskbuf,
                   int x0, int x1, int y0, int y1, int n_chan, bool additive,
                   const pdf14_nonseparable_blending_procs_t * pblend_procs,
                   bool has_matte, bool overprint, gx_color_index drawn_comps,
                   gs_memory_t *memory, gx_device *dev)
{
    int num_spots = tos->num_spots;
    uint16_t alpha = tos->alpha;
    uint16_t shape = tos->shape;
    gs_blend_mode_t blend_mode = tos->blend_mode;
    uint16_t *tos_ptr =
        (uint16_t *)(void *)(tos->data + (x0 - tos->rect.p.x)*2 +
                             (y0 - tos->rect.p.y) * tos->rowstride);
    uint16_t *nos_ptr =
        (uint16_t *)(void *)(nos->data + (x0 - nos->rect.p.x)*2 +
                             (y0 - nos->rect.p.y) * nos->rowstride);
    uint16_t *mask_row_ptr = NULL;
    int tos_planestride = tos->planestride;
    int nos_planestride = nos->planestride;
    uint16_t mask_bg_alpha = 0; /* Quiet compiler. */
    bool tos_isolated = tos->isolated;
    bool nos_isolated = nos->isolated;
    bool nos_knockout = nos->knockout;
    uint16_t *nos_alpha_g_ptr;
    int tos_shape_offset = n_chan * tos_planestride;
    int tos_alpha_g_offset = tos_shape_offset + (tos->has_shape ? tos_planestride : 0);
    bool tos_has_tag = tos->has_tags;
    int tos_tag_offset = tos_planestride * (tos->n_planes - 1);
    int nos_shape_offset = n_chan * nos_planestride;
    int nos_alpha_g_offset = nos_shape_offset + (nos->has_shape ? nos_planestride : 0);
    int nos_tag_offset = nos_planestride * (nos->n_planes - 1);
    const uint16_t *mask_tr_fn = NULL; /* Quiet compiler. */
    bool has_mask = false;
    uint16_t *backdrop_ptr = NULL;
    pdf14_device *pdev = (pdf14_device *)dev;
#if RAW_DUMP
    uint16_t *composed_ptr = NULL;
    int width = x1 - x0;
#endif
    art_pdf_compose_group16_fn fn;

    if ((tos->n_chan == 0) || (nos->n_chan == 0))
        return;
    rect_merge(nos->dirty, tos->dirty);
    if (nos->has_tags)
        if_debug7m('v', memory,
                   "pdf14_pop_transparency_group y0 = %d, y1 = %d, w = %d, alpha = %d, shape = %d, tag = %d, bm = %d\n",
                   y0, y1, x1 - x0, alpha, shape, dev->graphics_type_tag & ~GS_DEVICE_ENCODES_TAGS, blend_mode);
    else
        if_debug6m('v', memory,
                   "pdf14_pop_transparency_group y0 = %d, y1 = %d, w = %d, alpha = %d, shape = %d, bm = %d\n",
                   y0, y1, x1 - x0, alpha, shape, blend_mode);
    if (!nos->has_shape)
        nos_shape_offset = 0;
    if (!nos->has_tags)
        nos_tag_offset = 0;
    if (nos->has_alpha_g) {
        nos_alpha_g_ptr = nos_ptr + (nos_alpha_g_offset>>1);
    } else
        nos_alpha_g_ptr = NULL;
    if (nos->backdrop != NULL) {
        backdrop_ptr =
            (uint16_t *)(void *)(nos->backdrop + (x0 - nos->rect.p.x)*2 +
                                 (y0 - nos->rect.p.y) * nos->rowstride);
    }
    if (blend_mode != BLEND_MODE_Compatible && blend_mode != BLEND_MODE_Normal)
        overprint = false;

    if (maskbuf != NULL) {
        unsigned int tmp;
        mask_tr_fn = (uint16_t *)maskbuf->transfer_fn;
        /* Make sure we are in the mask buffer */
        if (maskbuf->data != NULL) {
            mask_row_ptr =
                (uint16_t *)(void *)(maskbuf->data + (x0 - maskbuf->rect.p.x)*2 +
                                     (y0 - maskbuf->rect.p.y) * maskbuf->rowstride);
            has_mask = true;
        }
        /* We may have a case, where we are outside the maskbuf rect. */
        /* We would have avoided creating the maskbuf->data */
        /* In that case, we should use the background alpha value */
        /* See discussion on the BC entry in the PDF spec.   */
        mask_bg_alpha = maskbuf->alpha;
        /* Adjust alpha by the mask background alpha.   This is only used
           if we are outside the soft mask rect during the filling operation */
        mask_bg_alpha = interp16(mask_tr_fn, mask_bg_alpha);
        tmp = alpha * mask_bg_alpha + 0x8000;
        mask_bg_alpha = (tmp + (tmp >> 8)) >> 8;
    }
    n_chan--; /* Now the true number of colorants (i.e. not including alpha)*/
#if RAW_DUMP
    composed_ptr = nos_ptr;
    dump_raw_buffer(memory, y1-y0, width, tos->n_planes, tos_planestride, tos->rowstride,
                    "bImageTOS", (byte *)tos_ptr, tos->deep);
    dump_raw_buffer(memory, y1-y0, width, nos->n_planes, nos_planestride, nos->rowstride,
                    "cImageNOS", (byte *)nos_ptr, tos->deep);
    if (maskbuf !=NULL && maskbuf->data != NULL) {
        dump_raw_buffer(memory, maskbuf->rect.q.y - maskbuf->rect.p.y,
                        maskbuf->rect.q.x - maskbuf->rect.p.x, maskbuf->n_planes,
                        maskbuf->planestride, maskbuf->rowstride, "dMask",
                        maskbuf->data, maskbuf->deep);
    }
#endif

    /* You might hope that has_mask iff maskbuf != NULL, but this is
     * not the case. Certainly we can see cases where maskbuf != NULL
     * and has_mask = 0. What's more, treating such cases as being
     * has_mask = 0 causes diffs. */
#ifdef TRACK_COMPOSE_GROUPS
    {
        int code = 0;

        code += !!nos_knockout;
        code += (!!nos_isolated)<<1;
        code += (!!tos_isolated)<<2;
        code += (!!tos->has_shape)<<3;
        code += (!!tos_has_tag)<<4;
        code += (!!additive)<<5;
        code += (!!overprint)<<6;
        code += (!!has_mask || maskbuf != NULL)<<7;
        code += (!!has_matte)<<8;
        code += (backdrop_ptr != NULL)<<9;
        code += (num_spots != 0)<<10;
        code += blend_mode<<11;

        if (track_compose_groups == 0)
        {
            atexit(dump_track_compose_groups);
            track_compose_groups = 1;
        }
        compose_groups[code]++;
    }
#endif

    /* We have tested the files on the cluster to see what percentage of
     * files/devices hit the different options. */
    if (nos_knockout)
        fn = &compose_group16_knockout; /* Small %ages, nothing more than 1.1% */
    else if (blend_mode != 0)
        fn = &compose_group16_nonknockout_blend; /* Small %ages, nothing more than 2% */
    else if (tos->has_shape == 0 && tos_has_tag == 0 && nos_isolated == 0 && nos_alpha_g_ptr == NULL &&
             nos_shape_offset == 0 && nos_tag_offset == 0 && backdrop_ptr == NULL && has_matte == 0 && num_spots == 0 &&
             overprint == 0) {
             /* Additive vs Subtractive makes no difference in normal blend mode with no spots */
        if (tos_isolated) {
            if (has_mask || maskbuf) {/* 7% */
                /* AirPrint test case hits this */
                if (maskbuf && maskbuf->rect.p.x <= x0 && maskbuf->rect.p.y <= y0 &&
                    maskbuf->rect.q.x >= x1 && maskbuf->rect.q.y >= y1)
                    fn = &compose_group16_nonknockout_nonblend_isolated_allmask_common;
                else
                    fn = &compose_group16_nonknockout_nonblend_isolated_mask_common;
            } else /* 14% */
                fn = &compose_group16_nonknockout_nonblend_isolated_nomask_common;
        } else {
            if (has_mask || maskbuf) /* 4% */
                fn = &compose_group16_nonknockout_nonblend_nonisolated_mask_common;
            else /* 15% */
                fn = &compose_group16_nonknockout_nonblend_nonisolated_nomask_common;
        }
    } else
        fn = compose_group16_nonknockout_noblend_general;

    tos_planestride >>= 1;
    tos_shape_offset >>= 1;
    tos_alpha_g_offset >>= 1;
    tos_tag_offset >>= 1;
    nos_planestride >>= 1;
    nos_shape_offset >>= 1;
    nos_tag_offset >>= 1;
    fn(tos_ptr, tos_isolated, tos_planestride, tos->rowstride>>1, alpha, shape, blend_mode, tos->has_shape,
                  tos_shape_offset, tos_alpha_g_offset, tos_tag_offset, tos_has_tag,
                  nos_ptr, nos_isolated, nos_planestride, nos->rowstride>>1, nos_alpha_g_ptr, nos_knockout,
                  nos_shape_offset, nos_tag_offset,
                  mask_row_ptr, has_mask, maskbuf, mask_bg_alpha, mask_tr_fn,
                  backdrop_ptr,
                  has_matte, n_chan, additive, num_spots, overprint, drawn_comps, x0, y0, x1, y1,
                  pblend_procs, pdev);

#if RAW_DUMP
    dump_raw_buffer(memory, y1-y0, width, nos->n_planes, nos_planestride<<1, nos->rowstride,
                    "eComposed", (byte *)composed_ptr, nos->deep);
    global_index++;
#endif
}