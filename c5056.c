gxht_thresh_image_init(gx_image_enum *penum)
{
    int code = 0;
    fixed ox;
    int temp;
    int dev_width, max_height;
    int spp_out;
    int k;
    gx_ht_order *d_order;
    gx_dda_fixed dda_ht;

    if (gx_device_must_halftone(penum->dev)) {
        if (penum->pgs != NULL && penum->pgs->dev_ht != NULL) {
            for (k = 0; k < penum->pgs->dev_ht->num_comp; k++) {
                d_order = &(penum->pgs->dev_ht->components[k].corder);
                code = gx_ht_construct_threshold(d_order, penum->dev,
                                                 penum->pgs, k);
                if (code < 0 ) {
                    return gs_rethrow(code, "threshold creation failed");
                }
            }
        } else {
            return -1;
        }
    }
    spp_out = penum->dev->color_info.num_components;
    /* Precompute values needed for rasterizing. */
    penum->dxx = float2fixed(penum->matrix.xx + fixed2float(fixed_epsilon) / 2);
    /* If the image is landscaped then we want to maintain a buffer
       that is sufficiently large so that we can hold a byte
       of halftoned data along the column.  This way we avoid doing
       multiple writes into the same position over and over.
       The size of the buffer we need depends upon the bitdepth of
       the output device, the number of device coloranants and the
       number of  colorants in the source space.  Note we will
       need to eventually  consider  multi-level halftone case
       here too.  For now, to make use of the SSE2 stuff, we would
       like to have a multiple of 16 bytes of data to process at a time.
       So we will collect the columns of data in a buffer that is LAND_BITS
       wide.  We will also keep track of the widths of each column.  When
       the total width count reaches LAND_BITS, we will create our
       threshold array and apply it.  We may have one column that is
       buffered between calls in this case.  Also if a call is made
       with h=0 we will flush the buffer as we are at the end of the
       data.  */
    if (penum->posture == image_landscape) {
        int col_length = fixed2int_var_rounded(any_abs(penum->x_extent.y));
        dda_ht = penum->dda.pixel0.y;
        if (penum->dxx > 0)
            dda_translate(dda_ht, -fixed_epsilon);      /* to match rounding in non-fast code */

        ox = dda_current(penum->dda.pixel0.x);
        temp = gxht_dda_length(&dda_ht, penum->rect.w);
        if (col_length < temp)
            col_length = temp;          /* choose max to make sure line_size is large enough */
        temp = (col_length + LAND_BITS)/LAND_BITS;      /* round up to allow for offset bits */
        /* bitmap_raster() expects the width in bits, hence "* 8" */
        penum->line_size = bitmap_raster((temp * LAND_BITS) * 8);  /* The stride */
        /* Now we need at most LAND_BITS of these */
        penum->line = gs_alloc_bytes(penum->memory,
                                     LAND_BITS * penum->line_size * spp_out + 16,
                                     "gxht_thresh");
        /* Same with this.  However, we only need one plane here */
        penum->thresh_buffer = gs_alloc_bytes(penum->memory,
                                           penum->line_size * LAND_BITS + 16,
                                           "gxht_thresh");
        /* That maps into (LAND_BITS/8) bytes of Halftone data */
        penum->ht_buffer =
                        gs_alloc_bytes(penum->memory,
                           penum->line_size * (LAND_BITS>>3) * spp_out,
                           "gxht_thresh");
        penum->ht_plane_height = penum->line_size;
        penum->ht_stride = penum->line_size;
        if (penum->line == NULL || penum->thresh_buffer == NULL
                    || penum->ht_buffer == NULL)
            return -1;
        penum->ht_landscape.count = 0;
        penum->ht_landscape.num_contones = 0;
        if (penum->y_extent.x < 0) {
            /* Going right to left */
            penum->ht_landscape.curr_pos = LAND_BITS-1;
            penum->ht_landscape.index = -1;
        } else {
            /* Going left to right */
            penum->ht_landscape.curr_pos = 0;
            penum->ht_landscape.index = 1;
        }
        if (penum->x_extent.y < 0) {
            penum->ht_landscape.flipy = true;
            penum->ht_landscape.y_pos =
                fixed2int_pixround_perfect(dda_current(penum->dda.pixel0.y) + penum->x_extent.y);
        } else {
            penum->ht_landscape.flipy = false;
            penum->ht_landscape.y_pos =
                fixed2int_pixround_perfect(dda_current(penum->dda.pixel0.y));
        }
        memset(&(penum->ht_landscape.widths[0]), 0, sizeof(int)*LAND_BITS);
        penum->ht_landscape.offset_set = false;
        penum->ht_offset_bits = 0; /* Will get set in call to render */
        if (code >= 0) {
#if defined(DEBUG) || defined(PACIFY_VALGRIND)
            memset(penum->line, 0, LAND_BITS * penum->line_size * spp_out + 16);
            memset(penum->ht_buffer, 0, penum->line_size * (LAND_BITS>>3) * spp_out);
            memset(penum->thresh_buffer, 0, LAND_BITS * penum->line_size + 16);
#endif
        }
    } else {
        /* In the portrait case we allocate a single line buffer
           in device width, a threshold buffer of the same size
           and possibly wider and the buffer for the halftoned
           bits. We have to do a bit of work to enable 16 byte
           boundary after an offset to ensure that we can make use
           of  the SSE2 operations for thresholding.  We do the
           allocations now to avoid doing them with every line */
        dda_ht = penum->dda.pixel0.x;
        if (penum->dxx > 0)
            dda_translate(dda_ht, -fixed_epsilon);      /* to match rounding in non-fast code */
        /* Initialize the ht_landscape stuff to zero */
        memset(&(penum->ht_landscape), 0, sizeof(ht_landscape_info_t));
        ox = dda_current(dda_ht);
        dev_width = gxht_dda_length(&dda_ht, penum->rect.w);
        /* Get the bit position so that we can do a copy_mono for
           the left remainder and then 16 bit aligned copies for the
           rest.  The right remainder will be OK as it will land in
           the MSBit positions. Note the #define chunk bits16 in
           gdevm1.c.  Allow also for a 15 sample over run.
        */
        penum->ht_offset_bits = (-fixed2int_var_rounded(ox)) & (bitmap_raster(1) - 1);
        if (penum->ht_offset_bits > 0) {
            penum->ht_stride = bitmap_raster((7 + (dev_width + 4)) + (ARCH_SIZEOF_LONG * 8));
        } else {
            penum->ht_stride = bitmap_raster((7 + (dev_width + 2)) + (ARCH_SIZEOF_LONG * 8));
        }
        /* We want to figure out the maximum height that we may
           have in taking a single source row and going to device
           space */
        max_height = (int) ceil(fixed2float(any_abs(penum->dst_height)) /
                                            (float) penum->Height);
        if ((max_height > 0) && (penum->ht_stride * spp_out > max_int / max_height))
            return -1;         /* overflow */

        penum->ht_buffer =
                        gs_alloc_bytes(penum->memory,
                           penum->ht_stride * max_height * spp_out,
                           "gxht_thresh");
        penum->ht_plane_height = penum->ht_stride * max_height;
        /* We want to have 128 bit alignement for our contone and
           threshold strips so that we can use SSE operations
           in the threshold operation.  Add in a minor buffer and offset
           to ensure this.  If gs_alloc_bytes provides at least 16
           bit alignment so we may need to move 14 bytes.  However, the
           HT process is split in two operations.  One that involves
           the HT of a left remainder and the rest which ensures that
           we pack in the HT data in the bits with no skew for a fast
           copy into the gdevm1 device (16 bit copies).  So, we
           need to account for those pixels which occur first and which
           are NOT aligned for the contone buffer.  After we offset
           by this remainder portion we should be 128 bit aligned.
           Also allow a 15 sample over run during the execution.  */
        temp = (int) ceil((float) ((dev_width + 15.0) + 15.0)/16.0);
        penum->line_size = bitmap_raster(temp * 16 * 8);  /* The stride */
        penum->line = gs_alloc_bytes(penum->memory, penum->line_size * spp_out,
                                     "gxht_thresh");
        penum->thresh_buffer = gs_alloc_bytes(penum->memory,
                                              penum->line_size * max_height,
                                              "gxht_thresh");
        if (penum->line == NULL || penum->thresh_buffer == NULL ||
            penum->ht_buffer == NULL) {
            return -1;
        } else {
#if defined(DEBUG) || defined(PACIFY_VALGRIND)
            memset(penum->line, 0, penum->line_size * spp_out);
            memset(penum->ht_buffer, 0, penum->ht_stride * max_height * spp_out);
            memset(penum->thresh_buffer, 0, penum->line_size * max_height);
#endif
        }
    }
    return code;
}