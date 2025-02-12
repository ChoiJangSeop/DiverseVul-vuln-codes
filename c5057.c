gx_image_enum_begin(gx_device * dev, const gs_gstate * pgs,
                    const gs_matrix *pmat, const gs_image_common_t * pic,
                const gx_drawing_color * pdcolor, const gx_clip_path * pcpath,
                gs_memory_t * mem, gx_image_enum *penum)
{
    const gs_pixel_image_t *pim = (const gs_pixel_image_t *)pic;
    gs_image_format_t format = pim->format;
    const int width = pim->Width;
    const int height = pim->Height;
    const int bps = pim->BitsPerComponent;
    bool masked = penum->masked;
    const float *decode = pim->Decode;
    gs_matrix_double mat;
    int index_bps;
    const gs_color_space *pcs = pim->ColorSpace;
    gs_logical_operation_t lop = (pgs ? pgs->log_op : lop_default);
    int code;
    int log2_xbytes = (bps <= 8 ? 0 : arch_log2_sizeof_frac);
    int spp, nplanes, spread;
    uint bsize;
    byte *buffer;
    fixed mtx, mty;
    gs_fixed_point row_extent, col_extent, x_extent, y_extent;
    bool device_color = true;
    gs_fixed_rect obox, cbox;
    bool gridfitimages = 0;
    bool in_pattern_accumulator = 0;
    int orthogonal;
    int force_interpolation = 0;

    penum->clues = NULL;
    penum->icc_setup.has_transfer = false;
    penum->icc_setup.is_lab = false;
    penum->icc_setup.must_halftone = false;
    penum->icc_setup.need_decode = false;
    penum->Width = width;
    penum->Height = height;

    if ((code = gx_image_compute_mat(pgs, pmat, &(pim->ImageMatrix), &mat)) < 0) {
        return code;
    }
    /* Grid fit: A common construction in postscript/PDF files is for images
     * to be constructed as a series of 'stacked' 1 pixel high images.
     * Furthermore, many of these are implemented as an imagemask plotted on
     * top of thin rectangles. The different fill rules for images and line
     * art produces problems; line art fills a pixel if any part of it is
     * touched - images only fill a pixel if the centre of the pixel is
     * covered. Bug 692666 is such a problem.
     *
     * As a workaround for this problem, the code below was introduced. The
     * concept is that orthogonal images can be 'grid fitted' (or 'stretch')
     * to entirely cover pixels that they touch. Initially I had this working
     * for all images regardless of type, but as testing has proceeded, this
     * showed more and more regressions, so I've cut the cases back in which
     * this code is used until it now only triggers on imagemasks that are
     * either 1 pixel high, or wide, and then not if we are rendering a
     * glyph (such as from a type3 font).
     */

    /* Ask the device if we are in a pattern accumulator */
    in_pattern_accumulator = (dev_proc(dev, dev_spec_op)(dev, gxdso_in_pattern_accumulator, NULL, 0));
    if (in_pattern_accumulator < 0)
        in_pattern_accumulator = 0;

    /* Figure out if we are orthogonal */
    if (mat.xy == 0 && mat.yx == 0)
        orthogonal = 1;
    else if (mat.xx == 0 && mat.yy == 0)
        orthogonal = 2;
    else
        orthogonal = 0;

    /* If we are in a pattern accumulator, we choose to always grid fit
     * orthogonal images. We do this by asking the device whether we
     * should grid fit. This allows us to avoid nasty blank lines around
     * the edges of cells.
     */
    gridfitimages = in_pattern_accumulator && orthogonal;

    if (pgs != NULL && pgs->show_gstate != NULL) {
        /* If we're a graphics state, and we're in a text object, then we
         * must be in a type3 font. Don't fiddle with it. */
    } else if (!gridfitimages &&
               (!penum->masked || penum->image_parent_type != 0)) {
        /* Other than for images we are specifically looking to grid fit (such as
         * ones in a pattern device), we only grid fit imagemasks */
    } else if (gridfitimages && (penum->masked && penum->image_parent_type == 0)) {
        /* We don't gridfit imagemasks in a pattern accumulator */
    } else if (pgs != NULL && pgs->fill_adjust.x == 0 && pgs->fill_adjust.y == 0) {
        /* If fill adjust is disabled, so is grid fitting */
    } else if (orthogonal == 1) {
        if (width == 1 || gridfitimages) {
            if (mat.xx > 0) {
                fixed ix0 = int2fixed(fixed2int(float2fixed(mat.tx)));
                double x1 = mat.tx + mat.xx * width;
                fixed ix1 = int2fixed(fixed2int_ceiling(float2fixed(x1)));
                mat.tx = (double)fixed2float(ix0);
                mat.xx = (double)(fixed2float(ix1 - ix0)/width);
            } else if (mat.xx < 0) {
                fixed ix0 = int2fixed(fixed2int_ceiling(float2fixed(mat.tx)));
                double x1 = mat.tx + mat.xx * width;
                fixed ix1 = int2fixed(fixed2int(float2fixed(x1)));
                mat.tx = (double)fixed2float(ix0);
                mat.xx = (double)(fixed2float(ix1 - ix0)/width);
            }
        }
        if (height == 1 || gridfitimages) {
            if (mat.yy > 0) {
                fixed iy0 = int2fixed(fixed2int(float2fixed(mat.ty)));
                double y1 = mat.ty + mat.yy * height;
                fixed iy1 = int2fixed(fixed2int_ceiling(float2fixed(y1)));
                mat.ty = (double)fixed2float(iy0);
                mat.yy = (double)(fixed2float(iy1 - iy0)/height);
            } else if (mat.yy < 0) {
                fixed iy0 = int2fixed(fixed2int_ceiling(float2fixed(mat.ty)));
                double y1 = mat.ty + mat.yy * height;
                fixed iy1 = int2fixed(fixed2int(float2fixed(y1)));
                mat.ty = (double)fixed2float(iy0);
                mat.yy = ((double)fixed2float(iy1 - iy0)/height);
            }
        }
    } else if (orthogonal == 2) {
        if (height == 1 || gridfitimages) {
            if (mat.yx > 0) {
                fixed ix0 = int2fixed(fixed2int(float2fixed(mat.tx)));
                double x1 = mat.tx + mat.yx * height;
                fixed ix1 = int2fixed(fixed2int_ceiling(float2fixed(x1)));
                mat.tx = (double)fixed2float(ix0);
                mat.yx = (double)(fixed2float(ix1 - ix0)/height);
            } else if (mat.yx < 0) {
                fixed ix0 = int2fixed(fixed2int_ceiling(float2fixed(mat.tx)));
                double x1 = mat.tx + mat.yx * height;
                fixed ix1 = int2fixed(fixed2int(float2fixed(x1)));
                mat.tx = (double)fixed2float(ix0);
                mat.yx = (double)(fixed2float(ix1 - ix0)/height);
            }
        }
        if (width == 1 || gridfitimages) {
            if (mat.xy > 0) {
                fixed iy0 = int2fixed(fixed2int(float2fixed(mat.ty)));
                double y1 = mat.ty + mat.xy * width;
                fixed iy1 = int2fixed(fixed2int_ceiling(float2fixed(y1)));
                mat.ty = (double)fixed2float(iy0);
                mat.xy = (double)(fixed2float(iy1 - iy0)/width);
            } else if (mat.xy < 0) {
                fixed iy0 = int2fixed(fixed2int_ceiling(float2fixed(mat.ty)));
                double y1 = mat.ty + mat.xy * width;
                fixed iy1 = int2fixed(fixed2int(float2fixed(y1)));
                mat.ty = (double)fixed2float(iy0);
                mat.xy = ((double)fixed2float(iy1 - iy0)/width);
            }
        }
    }

    /* When rendering to a pattern accumulator, if we are downscaling
     * then enable interpolation, as otherwise dropouts can cause
     * serious problems. */
    if (in_pattern_accumulator) {
        double ome = ((double)(fixed_1 - fixed_epsilon)) / (double)fixed_1; /* One Minus Epsilon */

        if (orthogonal == 1) {
            if ((mat.xx > -ome && mat.xx < ome) || (mat.yy > -ome && mat.yy < ome)) {
                force_interpolation = true;
            }
        } else if (orthogonal == 2) {
            if ((mat.xy > -ome && mat.xy < ome) || (mat.yx > -ome && mat.yx < ome)) {
                force_interpolation = true;
            }
        }
    }

    /* Can we restrict the amount of image we need? */
    while (pcpath) /* So we can break out of it */
    {
        gs_rect rect, rect_out;
        gs_matrix mi;
        const gs_matrix *m = pgs != NULL ? &ctm_only(pgs) : NULL;
        gs_fixed_rect obox;
        gs_int_rect irect;
        if (m == NULL || (code = gs_matrix_invert(m, &mi)) < 0 ||
            (code = gs_matrix_multiply(&mi, &pic->ImageMatrix, &mi)) < 0) {
            /* Give up trying to shrink the render box, but continue processing */
            break;
        }
        gx_cpath_outer_box(pcpath, &obox);
        rect.p.x = fixed2float(obox.p.x);
        rect.p.y = fixed2float(obox.p.y);
        rect.q.x = fixed2float(obox.q.x);
        rect.q.y = fixed2float(obox.q.y);
        code = gs_bbox_transform(&rect, &mi, &rect_out);
        if (code < 0) {
            /* Give up trying to shrink the render box, but continue processing */
            break;
        }
        irect.p.x = (int)(rect_out.p.x-1.0);
        irect.p.y = (int)(rect_out.p.y-1.0);
        irect.q.x = (int)(rect_out.q.x+1.0);
        irect.q.y = (int)(rect_out.q.y+1.0);
        /* Need to expand the region to allow for the fact that the mitchell
         * scaler reads multiple pixels in. This calculation can probably be
         * improved. */
        /* If mi.{xx,yy} > 1 then we are downscaling. During downscaling,
         * the support increases to ensure that we don't lose pixels contributions
         * entirely. */
        /* I do not understand the need for the +/- 1 fudge factors,
         * but they seem to be required. Increasing the render rectangle can
         * never be bad at least... RJW */
        {
            float support = any_abs(mi.xx);
            int isupport;
            if (any_abs(mi.yy) > support)
                support = any_abs(mi.yy);
            if (any_abs(mi.xy) > support)
                support = any_abs(mi.xy);
            if (any_abs(mi.yx) > support)
                support = any_abs(mi.yx);
            isupport = (int)(MAX_ISCALE_SUPPORT * (support+1)) + 1;
            irect.p.x -= isupport;
            irect.p.y -= isupport;
            irect.q.x += isupport;
            irect.q.y += isupport;
        }
        if (penum->rrect.x < irect.p.x) {
            penum->rrect.w -= irect.p.x - penum->rrect.x;
            if (penum->rrect.w < 0)
               penum->rrect.w = 0;
            penum->rrect.x = irect.p.x;
        }
        if (penum->rrect.x + penum->rrect.w > irect.q.x) {
            penum->rrect.w = irect.q.x - penum->rrect.x;
            if (penum->rrect.w < 0)
                penum->rrect.w = 0;
        }
        if (penum->rrect.y < irect.p.y) {
            penum->rrect.h -= irect.p.y - penum->rrect.y;
            if (penum->rrect.h < 0)
                penum->rrect.h = 0;
            penum->rrect.y = irect.p.y;
        }
        if (penum->rrect.y + penum->rrect.h > irect.q.y) {
            penum->rrect.h = irect.q.y - penum->rrect.y;
            if (penum->rrect.h < 0)
                penum->rrect.h = 0;
        }
        break; /* Out of the while */
    }
    /* Check for the intersection being null */
    if (penum->rrect.x + penum->rrect.w <= penum->rect.x  ||
        penum->rect.x  + penum->rect.w  <= penum->rrect.x ||
        penum->rrect.y + penum->rrect.h <= penum->rect.y  ||
        penum->rect.y  + penum->rect.h  <= penum->rrect.y)
    {
          /* Something may have gone wrong with the floating point above.
           * set the region to something sane. */
        penum->rrect.x = penum->rect.x;
        penum->rrect.y = penum->rect.y;
        penum->rrect.w = 0;
        penum->rrect.h = 0;
    }

    /*penum->matrix = mat;*/
    penum->matrix.xx = mat.xx;
    penum->matrix.xy = mat.xy;
    penum->matrix.yx = mat.yx;
    penum->matrix.yy = mat.yy;
    penum->matrix.tx = mat.tx;
    penum->matrix.ty = mat.ty;
    if_debug6m('b', mem, " [%g %g %g %g %g %g]\n",
              mat.xx, mat.xy, mat.yx, mat.yy, mat.tx, mat.ty);
    /* following works for 1, 2, 4, 8, 12, 16 */
    index_bps = (bps < 8 ? bps >> 1 : (bps >> 2) + 1);
    /*
     * Compute extents with distance transformation.
     */
    if (mat.tx > 0)
        mtx = float2fixed(mat.tx);
    else { /* Use positive values to ensure round down. */
        int f = (int)-mat.tx + 1;

        mtx = float2fixed(mat.tx + f) - int2fixed(f);
    }
    if (mat.ty > 0)
        mty = float2fixed(mat.ty);
    else {  /* Use positive values to ensure round down. */
        int f = (int)-mat.ty + 1;

        mty = float2fixed(mat.ty + f) - int2fixed(f);
    }

    row_extent.x = float2fixed_rounded_boxed(width * mat.xx);
    row_extent.y =
        (is_fzero(mat.xy) ? fixed_0 :
         float2fixed_rounded_boxed(width * mat.xy));
    col_extent.x =
        (is_fzero(mat.yx) ? fixed_0 :
         float2fixed_rounded_boxed(height * mat.yx));
    col_extent.y = float2fixed_rounded_boxed(height * mat.yy);
    gx_image_enum_common_init((gx_image_enum_common_t *)penum,
                              (const gs_data_image_t *)pim,
                              &image1_enum_procs, dev,
                              (masked ? 1 : (penum->alpha ? cs_num_components(pcs)+1 : cs_num_components(pcs))),
                              format);
    if (penum->rect.w == width && penum->rect.h == height) {
        x_extent = row_extent;
        y_extent = col_extent;
    } else {
        int rw = penum->rect.w, rh = penum->rect.h;

        x_extent.x = float2fixed_rounded_boxed(rw * mat.xx);
        x_extent.y =
            (is_fzero(mat.xy) ? fixed_0 :
             float2fixed_rounded_boxed(rw * mat.xy));
        y_extent.x =
            (is_fzero(mat.yx) ? fixed_0 :
             float2fixed_rounded_boxed(rh * mat.yx));
        y_extent.y = float2fixed_rounded_boxed(rh * mat.yy);
    }
    /* Set icolor0 and icolor1 to point to image clues locations if we have
       1spp or an imagemask, otherwise image clues is not used and
       we have these values point to other member variables */
    if (masked || cs_num_components(pcs) == 1) {
        /* Go ahead and allocate now if not already done.  For a mask
           we really should only do 2 values. For now, the goal is to
           eliminate the 256 bytes for the >8bpp image enumerator */
        penum->clues = (gx_image_clue*) gs_alloc_bytes(mem, sizeof(gx_image_clue)*256,
                             "gx_image_enum_begin");
        if (penum->clues == NULL)
            return_error(gs_error_VMerror);
        penum->icolor0 = &(penum->clues[0].dev_color);
        penum->icolor1 = &(penum->clues[255].dev_color);
    } else {
        penum->icolor0 = &(penum->icolor0_val);
        penum->icolor1 = &(penum->icolor1_val);
    }
    if (masked) {       /* This is imagemask. */
        if (bps != 1 || pcs != NULL || penum->alpha || decode[0] == decode[1]) {
            return_error(gs_error_rangecheck);
        }
        /* Initialize color entries 0 and 255. */
        set_nonclient_dev_color(penum->icolor0, gx_no_color_index);
        set_nonclient_dev_color(penum->icolor1, gx_no_color_index);
        *(penum->icolor1) = *pdcolor;
        memcpy(&penum->map[0].table.lookup4x1to32[0],
               (decode[0] < decode[1] ? lookup4x1to32_inverted :
                lookup4x1to32_identity),
               16 * 4);
        penum->map[0].decoding = sd_none;
        spp = 1;
        lop = rop3_know_S_0(lop);
    } else {                    /* This is image, not imagemask. */
        const gs_color_space_type *pcst = pcs->type;
        int b_w_color;

        spp = cs_num_components(pcs);
        if (spp < 0) {          /* Pattern not allowed */
            return_error(gs_error_rangecheck);
        }
        if (penum->alpha)
            ++spp;
        /* Use a less expensive format if possible. */
        switch (format) {
        case gs_image_format_bit_planar:
            if (bps > 1)
                break;
            format = gs_image_format_component_planar;
        case gs_image_format_component_planar:
            if (spp == 1)
                format = gs_image_format_chunky;
        default:                /* chunky */
            break;
        }

        if (pcs->cmm_icc_profile_data != NULL) {
            device_color = false;
        } else {
            device_color = (*pcst->concrete_space) (pcs, pgs) == pcs;
        }

        code = image_init_colors(penum, bps, spp, format, decode, pgs, dev,
                          pcs, &device_color);
        if (code < 0) 
            return gs_throw(code, "Image colors initialization failed");
        /* If we have a CIE based color space and the icc equivalent profile
           is not yet set, go ahead and handle that now.  It may already
           be done due to the above init_colors which may go through remap. */
        if (gs_color_space_is_PSCIE(pcs) && pcs->icc_equivalent == NULL) {
            code = gs_colorspace_set_icc_equivalent((gs_color_space *)pcs, &(penum->icc_setup.is_lab),
                                                pgs->memory);
            if (code < 0)
                return code;
            if (penum->icc_setup.is_lab) {
                /* Free what ever profile was created and use the icc manager's
                   cielab profile */
                gs_color_space *curr_pcs = (gs_color_space *)pcs;
                rc_decrement(curr_pcs->icc_equivalent,"gx_image_enum_begin");
                rc_decrement(curr_pcs->cmm_icc_profile_data,"gx_image_enum_begin");
                curr_pcs->cmm_icc_profile_data = pgs->icc_manager->lab_profile;
                rc_increment(curr_pcs->cmm_icc_profile_data);
            }
        }
        /* Try to transform non-default RasterOps to something */
        /* that we implement less expensively. */
        if (!pim->CombineWithColor)
            lop = rop3_know_T_0(lop) & ~lop_T_transparent;
        else if ((rop3_uses_T(lop) && color_draws_b_w(dev, pdcolor) == 0))
            lop = rop3_know_T_0(lop);

        if (lop != rop3_S &&    /* if best case, no more work needed */
            !rop3_uses_T(lop) && bps == 1 && spp == 1 &&
            (b_w_color =
             color_draws_b_w(dev, penum->icolor0)) >= 0 &&
            color_draws_b_w(dev, penum->icolor1) == (b_w_color ^ 1)
            ) {
            if (b_w_color) {    /* Swap the colors and invert the RasterOp source. */
                gx_device_color dcolor;

                dcolor = *(penum->icolor0);
                *(penum->icolor0) = *(penum->icolor1);
                *(penum->icolor1) = dcolor;
                lop = rop3_invert_S(lop);
            }
            /*
             * At this point, we know that the source pixels
             * correspond directly to the S input for the raster op,
             * i.e., icolor0 is black and icolor1 is white.
             */
            switch (lop) {
                case rop3_D & rop3_S:
                    /* Implement this as an inverted mask writing 0s. */
                    *(penum->icolor1) = *(penum->icolor0);
                    /* (falls through) */
                case rop3_D | rop3_not(rop3_S):
                    /* Implement this as an inverted mask writing 1s. */
                    memcpy(&penum->map[0].table.lookup4x1to32[0],
                           lookup4x1to32_inverted, 16 * 4);
                  rmask:        /* Fill in the remaining parameters for a mask. */
                    penum->masked = masked = true;
                    set_nonclient_dev_color(penum->icolor0, gx_no_color_index);
                    penum->map[0].decoding = sd_none;
                    lop = rop3_T;
                    break;
                case rop3_D & rop3_not(rop3_S):
                    /* Implement this as a mask writing 0s. */
                    *(penum->icolor1) = *(penum->icolor0);
                    /* (falls through) */
                case rop3_D | rop3_S:
                    /* Implement this as a mask writing 1s. */
                    memcpy(&penum->map[0].table.lookup4x1to32[0],
                           lookup4x1to32_identity, 16 * 4);
                    goto rmask;
                default:
                    ;
            }
        }
    }
    penum->device_color = device_color;
    /*
     * Adjust width upward for unpacking up to 7 trailing bits in
     * the row, plus 1 byte for end-of-run, plus up to 7 leading
     * bits for data_x offset within a packed byte.
     */
    bsize = ((bps > 8 ? width * 2 : width) + 15) * spp;
    buffer = gs_alloc_bytes(mem, bsize, "image buffer");
    if (buffer == 0) {
        return_error(gs_error_VMerror);
    }
    penum->bps = bps;
    penum->unpack_bps = bps;
    penum->log2_xbytes = log2_xbytes;
    penum->spp = spp;
    switch (format) {
    case gs_image_format_chunky:
        nplanes = 1;
        spread = 1 << log2_xbytes;
        break;
    case gs_image_format_component_planar:
        nplanes = spp;
        spread = spp << log2_xbytes;
        break;
    case gs_image_format_bit_planar:
        nplanes = spp * bps;
        spread = spp << log2_xbytes;
        break;
    default:
        /* No other cases are possible (checked by gx_image_enum_alloc). */
        return_error(gs_error_Fatal);
    }
    penum->num_planes = nplanes;
    penum->spread = spread;
    /*
     * If we're asked to interpolate in a partial image, we have to
     * assume that the client either really only is interested in
     * the given sub-image, or else is constructing output out of
     * overlapping pieces.
     */
    penum->interpolate = force_interpolation ? interp_force : pim->Interpolate ? interp_on : interp_off;
    penum->x_extent = x_extent;
    penum->y_extent = y_extent;
    penum->posture =
        ((x_extent.y | y_extent.x) == 0 ? image_portrait :
         (x_extent.x | y_extent.y) == 0 ? image_landscape :
         image_skewed);
    penum->pgs = pgs;
    penum->pcs = pcs;
    penum->memory = mem;
    penum->buffer = buffer;
    penum->buffer_size = bsize;
    penum->line = 0;
    penum->icc_link = NULL;
    penum->color_cache = NULL;
    penum->ht_buffer = NULL;
    penum->thresh_buffer = NULL;
    penum->use_cie_range = false;
    penum->line_size = 0;
    penum->use_rop = lop != (masked ? rop3_T : rop3_S);
#ifdef DEBUG
    if (gs_debug_c('*')) {
        if (penum->use_rop)
            dmprintf1(mem, "[%03x]", lop);
        dmprintf5(mem, "%c%d%c%dx%d ",
                 (masked ? (color_is_pure(pdcolor) ? 'm' : 'h') : 'i'),
                 bps,
                 (penum->posture == image_portrait ? ' ' :
                  penum->posture == image_landscape ? 'L' : 'T'),
                 width, height);
    }
#endif
    penum->slow_loop = 0;
    if (pcpath == 0) {
        (*dev_proc(dev, get_clipping_box)) (dev, &obox);
        cbox = obox;
        penum->clip_image = 0;
    } else
        penum->clip_image =
            (gx_cpath_outer_box(pcpath, &obox) |        /* not || */
             gx_cpath_inner_box(pcpath, &cbox) ?
             0 : image_clip_region);
    penum->clip_outer = obox;
    penum->clip_inner = cbox;
    penum->log_op = rop3_T;     /* rop device takes care of this */
    penum->clip_dev = 0;        /* in case we bail out */
    penum->rop_dev = 0;         /* ditto */
    penum->scaler = 0;          /* ditto */
    /*
     * If all four extrema of the image fall within the clipping
     * rectangle, clipping is never required.  When making this check,
     * we must carefully take into account the fact that we only care
     * about pixel centers.
     */
    {
        fixed
            epx = min(row_extent.x, 0) + min(col_extent.x, 0),
            eqx = max(row_extent.x, 0) + max(col_extent.x, 0),
            epy = min(row_extent.y, 0) + min(col_extent.y, 0),
            eqy = max(row_extent.y, 0) + max(col_extent.y, 0);

        {
            int hwx, hwy;

            switch (penum->posture) {
                case image_portrait:
                    hwx = width, hwy = height;
                    break;
                case image_landscape:
                    hwx = height, hwy = width;
                    break;
                default:
                    hwx = hwy = 0;
            }
            /*
             * If the image is only 1 sample wide or high,
             * and is less than 1 device pixel wide or high,
             * move it slightly so that it covers pixel centers.
             * This is a hack to work around a bug in some old
             * versions of TeX/dvips, which use 1-bit-high images
             * to draw horizontal and vertical lines without
             * positioning them properly.
             */
            if (hwx == 1 && eqx - epx < fixed_1) {
                fixed diff =
                arith_rshift_1(row_extent.x + col_extent.x);

                mtx = (((mtx + diff) | fixed_half) & -fixed_half) - diff;
            }
            if (hwy == 1 && eqy - epy < fixed_1) {
                fixed diff =
                arith_rshift_1(row_extent.y + col_extent.y);

                mty = (((mty + diff) | fixed_half) & -fixed_half) - diff;
            }
        }
        if_debug5m('b', mem, "[b]Image: %sspp=%d, bps=%d, mt=(%g,%g)\n",
                   (masked? "masked, " : ""), spp, bps,
                   fixed2float(mtx), fixed2float(mty));
        if_debug9m('b', mem,
                   "[b]   cbox=(%g,%g),(%g,%g), obox=(%g,%g),(%g,%g), clip_image=0x%x\n",
                   fixed2float(cbox.p.x), fixed2float(cbox.p.y),
                   fixed2float(cbox.q.x), fixed2float(cbox.q.y),
                   fixed2float(obox.p.x), fixed2float(obox.p.y),
                   fixed2float(obox.q.x), fixed2float(obox.q.y),
                   penum->clip_image);
        /* These DDAs enumerate the starting position of each source pixel
         * row in device space. */
        dda_init(penum->dda.row.x, mtx, col_extent.x, height);
        dda_init(penum->dda.row.y, mty, col_extent.y, height);
        if (penum->posture == image_portrait) {
            penum->dst_width = row_extent.x;
            penum->dst_height = col_extent.y;
        } else {
            penum->dst_width = col_extent.x;
            penum->dst_height = row_extent.y;
        }
        /* For gs_image_class_0_interpolate. */
        penum->yi0 = fixed2int_pixround_perfect(dda_current(penum->dda.row.y)); /* For gs_image_class_0_interpolate. */
        if (penum->rect.y) {
            int y = penum->rect.y;

            while (y--) {
                dda_next(penum->dda.row.x);
                dda_next(penum->dda.row.y);
            }
        }
        penum->cur.x = penum->prev.x = dda_current(penum->dda.row.x);
        penum->cur.y = penum->prev.y = dda_current(penum->dda.row.y);
        /* These DDAs enumerate the starting positions of each row of our
         * source pixel data, in the subrectangle ('strip') that we are
         * actually rendering. */
        dda_init(penum->dda.strip.x, penum->cur.x, row_extent.x, width);
        dda_init(penum->dda.strip.y, penum->cur.y, row_extent.y, width);
        if (penum->rect.x) {
            dda_advance(penum->dda.strip.x, penum->rect.x);
            dda_advance(penum->dda.strip.y, penum->rect.x);
        }
        {
            fixed ox = dda_current(penum->dda.strip.x);
            fixed oy = dda_current(penum->dda.strip.y);

            if (!penum->clip_image)     /* i.e., not clip region */
                penum->clip_image =
                    (fixed_pixround(ox + epx) < fixed_pixround(cbox.p.x) ?
                     image_clip_xmin : 0) +
                    (fixed_pixround(ox + eqx) >= fixed_pixround(cbox.q.x) ?
                     image_clip_xmax : 0) +
                    (fixed_pixround(oy + epy) < fixed_pixround(cbox.p.y) ?
                     image_clip_ymin : 0) +
                    (fixed_pixround(oy + eqy) >= fixed_pixround(cbox.q.y) ?
                     image_clip_ymax : 0);
        }
    }
    penum->y = 0;
    penum->used.x = 0;
    penum->used.y = 0;
    {
        static sample_unpack_proc_t procs[2][6] = {
        {   sample_unpack_1, sample_unpack_2,
            sample_unpack_4, sample_unpack_8,
            sample_unpack_12, sample_unpack_16
        },
        {   sample_unpack_1_interleaved, sample_unpack_2_interleaved,
            sample_unpack_4_interleaved, sample_unpack_8_interleaved,
            sample_unpack_12, sample_unpack_16
        }};
        int num_planes = penum->num_planes;
        bool interleaved = (num_planes == 1 && penum->plane_depths[0] != penum->bps);
        int i;

        if (interleaved) {
            int num_components = penum->plane_depths[0] / penum->bps;

            for (i = 1; i < num_components; i++) {
                if (decode[0] != decode[i * 2 + 0] ||
                    decode[1] != decode[i * 2 + 1])
                    break;
            }
            if (i == num_components)
                interleaved = false; /* Use single table. */
        }
        penum->unpack = procs[interleaved][index_bps];

        if_debug1m('b', mem, "[b]unpack=%d\n", bps);
        /* Set up pixel0 for image class procedures. */
        penum->dda.pixel0 = penum->dda.strip;
        for (i = 0; i < gx_image_class_table_count; ++i)
            if ((penum->render = gx_image_class_table[i](penum)) != 0)
                break;
        if (i == gx_image_class_table_count) {
            /* No available class can handle this image. */
            gx_default_end_image(dev, (gx_image_enum_common_t *) penum,
                                 false);
            return_error(gs_error_rangecheck);
        }
    }
    if (penum->clip_image && pcpath) {  /* Set up the clipping device. */
        gx_device_clip *cdev =
            gs_alloc_struct(mem, gx_device_clip,
                            &st_device_clip, "image clipper");

        if (cdev == 0) {
            gx_default_end_image(dev,
                                 (gx_image_enum_common_t *) penum,
                                 false);
            return_error(gs_error_VMerror);
        }
        gx_make_clip_device_in_heap(cdev, pcpath, dev, mem);
        penum->clip_dev = cdev;
    }
    if (penum->use_rop) {       /* Set up the RasterOp source device. */
        gx_device_rop_texture *rtdev;

        code = gx_alloc_rop_texture_device(&rtdev, mem,
                                           "image RasterOp");
        if (code < 0) {
            gx_default_end_image(dev, (gx_image_enum_common_t *) penum,
                                 false);
            return code;
        }
        /* The 'target' must not be NULL for gx_make_rop_texture_device */
        if (!penum->clip_dev && !dev)
            return_error(gs_error_undefined);

        gx_make_rop_texture_device(rtdev,
                                   (penum->clip_dev != 0 ?
                                    (gx_device *) penum->clip_dev :
                                    dev), lop, pdcolor);
        gx_device_retain((gx_device *)rtdev, true);
        penum->rop_dev = rtdev;
    }
    return 0;
}