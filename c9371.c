tiffsep1_print_page(gx_device_printer * pdev, gp_file * file)
{
    tiffsep1_device * const tfdev = (tiffsep1_device *)pdev;
    int num_std_colorants = tfdev->devn_params.num_std_colorant_names;
    int num_order = tfdev->devn_params.num_separation_order_names;
    int num_spot = tfdev->devn_params.separations.num_separations;
    int num_comp, comp_num, code = 0, code1 = 0;
    short map_comp_to_sep[GX_DEVICE_COLOR_MAX_COMPONENTS];
    char *name = NULL;
    int save_depth = pdev->color_info.depth;
    int save_numcomps = pdev->color_info.num_components;
    const char *fmt;
    gs_parsed_file_name_t parsed;
    int non_encodable_count = 0;

    if (tfdev->thresholds[0].dstart == NULL)
        return_error(gs_error_rangecheck);

    name = (char *)gs_alloc_bytes(pdev->memory, gp_file_name_sizeof, "tiffsep1_print_page(name)");
    if (!name)
        return_error(gs_error_VMerror);

    build_comp_to_sep_map((tiffsep_device *)tfdev, map_comp_to_sep);

    /*
     * Since different pages may have different spot colors, if this is for a
     * page after Page 1, we require that each output file is unique with a "fmt"
     * (i.e. %d) as part of the filename. We create individual separation files
     * for each page of the input.
     * Since the TIFF lib requires seeakable files, /dev/null or nul: are
     * not allowed (as they are with the psdcmyk devices).
    */
    code = gx_parse_output_file_name(&parsed, &fmt, tfdev->fname,
                                     strlen(tfdev->fname), pdev->memory);
    if (code < 0 || (fmt == NULL && tfdev->PageCount > 0)) {
       emprintf(tfdev->memory,
                "\nUse of the %%d format is required to output more than one page to tiffsep1.\n"
                "See doc/Devices.htm#TIFF for details.\n\n");
       code = gs_note_error(gs_error_ioerror);
    }
    /* If the output file is on disk and the name contains a page #, */
    /* then delete the previous file. */
    if (pdev->file != NULL && parsed.iodev == iodev_default(pdev->memory) && fmt) {
        long count1 = pdev->PageCount;
        char *compname = (char *)gs_alloc_bytes(pdev->memory, gp_file_name_sizeof, "tiffsep1_print_page(compname)");
        if (!compname) {
            code = gs_note_error(gs_error_VMerror);
            goto done;
        }

        gx_device_close_output_file((gx_device *)pdev, pdev->fname, pdev->file);
        pdev->file = NULL;

        while (*fmt != 'l' && *fmt != '%')
            --fmt;
        if (*fmt == 'l')
            gs_sprintf(compname, parsed.fname, count1);
        else
            gs_sprintf(compname, parsed.fname, (int)count1);
        parsed.iodev->procs.delete_file(parsed.iodev, compname);
        /* we always need an open printer (it will get deleted in tiffsep1_prn_close */
        code = gdev_prn_open_printer((gx_device *)pdev, 1);

        gs_free_object(pdev->memory, compname, "tiffsep_print_page(compname)");
        if (code < 0) {
            goto done;
        }
    }

    /* Set up the separation output files */
    num_comp = number_output_separations( tfdev->color_info.num_components,
                                        num_std_colorants, num_order, num_spot);
    for (comp_num = 0; comp_num < num_comp; comp_num++ ) {
        int sep_num = map_comp_to_sep[comp_num];

        code = create_separation_file_name((tiffsep_device *)tfdev, name,
                                        gp_file_name_sizeof, sep_num, true);
        if (code < 0) {
            goto done;
        }

        /* Open the separation file, if not already open */
        if (tfdev->sep_file[comp_num] == NULL) {
            code = gs_add_outputfile_control_path(tfdev->memory, name);
            if (code < 0) {
                goto done;
            }
            code = gx_device_open_output_file((gx_device *)pdev, name,
                    true, true, &(tfdev->sep_file[comp_num]));
            if (code < 0) {
                goto done;
            }
            tfdev->tiff[comp_num] = tiff_from_filep(pdev, name,
                                                    tfdev->sep_file[comp_num],
                                                    tfdev->BigEndian, tfdev->UseBigTIFF);
            if (!tfdev->tiff[comp_num]) {
                code = gs_note_error(gs_error_ioerror);
                goto done;
            }
        }

        pdev->color_info.depth = 8;     /* Create files for 8 bit gray */
        pdev->color_info.num_components = 1;
        code = tiff_set_fields_for_printer(pdev, tfdev->tiff[comp_num], 1, 0, tfdev->write_datetime);
        tiff_set_gray_fields(pdev, tfdev->tiff[comp_num], 1, tfdev->Compression, tfdev->MaxStripSize);
        pdev->color_info.depth = save_depth;
        pdev->color_info.num_components = save_numcomps;
        if (code < 0) {
            goto done;
        }

    }   /* end initialization of separation files */


    {   /* Get the expanded contone line, halftone and write out the dithered separations */
        byte *planes[GS_CLIENT_COLOR_MAX_COMPONENTS];
        int width = tfdev->width;
        int raster_plane = bitmap_raster(width * 8);
        int dithered_raster = ((7 + width) / 8) + ARCH_SIZEOF_LONG;
        int pixel, y;
        gs_get_bits_params_t params;
        gs_int_rect rect;
        /* the dithered_line is assumed to be 32-bit aligned by the alloc */
        uint32_t *dithered_line = (uint32_t *)gs_alloc_bytes(pdev->memory, dithered_raster,
                                "tiffsep1_print_page");

        memset(planes, 0, sizeof(*planes) * GS_CLIENT_COLOR_MAX_COMPONENTS);

        /* Return planar data */
        params.options = (GB_RETURN_POINTER | GB_RETURN_COPY |
             GB_ALIGN_STANDARD | GB_OFFSET_0 | GB_RASTER_STANDARD |
             GB_PACKING_PLANAR | GB_COLORS_NATIVE | GB_ALPHA_NONE);
        params.x_offset = 0;
        params.raster = bitmap_raster(width * pdev->color_info.depth);

        code = 0;
        for (comp_num = 0; comp_num < num_comp; comp_num++) {
            planes[comp_num] = gs_alloc_bytes(pdev->memory, raster_plane,
                                            "tiffsep1_print_page");
            if (planes[comp_num] == NULL) {
                code = gs_error_VMerror;
                break;
            }
        }

        if (code < 0 || dithered_line == NULL) {
            code = gs_note_error(gs_error_VMerror);
            goto cleanup;
        }

        for (comp_num = 0; comp_num < num_comp; comp_num++ )
            TIFFCheckpointDirectory(tfdev->tiff[comp_num]);

        rect.p.x = 0;
        rect.q.x = pdev->width;
        /* Loop for the lines */
        for (y = 0; y < pdev->height; ++y) {
            rect.p.y = y;
            rect.q.y = y + 1;
            /* We have to reset the pointers since get_bits_rect will have moved them */
            for (comp_num = 0; comp_num < num_comp; comp_num++)
                params.data[comp_num] = planes[comp_num];
            code = (*dev_proc(pdev, get_bits_rectangle))((gx_device *)pdev, &rect, &params, NULL);
            if (code < 0)
                break;

            /* Dither the separation and write it out */
            for (comp_num = 0; comp_num < num_comp; comp_num++ ) {

/***** #define SKIP_HALFTONING_FOR_TIMING *****/ /* uncomment for timing test */
#ifndef SKIP_HALFTONING_FOR_TIMING

                /*
                 * Define 32-bit writes by default. Testing shows that while this is more
                 * complex code, it runs measurably and consistently faster than the more
                 * obvious 8-bit code. The 8-bit code is kept to help future optimization
                 * efforts determine what affects tight loop optimization. Subtracting the
                 * time when halftoning is skipped shows that the 32-bit halftoning is
                 * 27% faster.
                 */
#define USE_32_BIT_WRITES
                byte *thresh_line_base = tfdev->thresholds[comp_num].dstart +
                                    ((y % tfdev->thresholds[comp_num].dheight) *
                                        tfdev->thresholds[comp_num].dwidth) ;
                byte *thresh_ptr = thresh_line_base;
                byte *thresh_limit = thresh_ptr + tfdev->thresholds[comp_num].dwidth;
                byte *src = params.data[comp_num];
#ifdef USE_32_BIT_WRITES
                uint32_t *dest = dithered_line;
                uint32_t val = 0;
                const uint32_t *mask = &bit_order[0];
#else   /* example 8-bit code */
                byte *dest = dithered_line;
                byte val = 0;
                byte mask = 0x80;
#endif /* USE_32_BIT_WRITES */

                for (pixel = 0; pixel < width; pixel++, src++) {
#ifdef USE_32_BIT_WRITES
                    if (*src < *thresh_ptr++)
                        val |= *mask;
                    if (++mask == &(bit_order[32])) {
                        *dest++ = val;
                        val = 0;
                        mask = &bit_order[0];
                    }
#else   /* example 8-bit code */
                    if (*src < *thresh_ptr++)
                        val |= mask;
                    mask >>= 1;
                    if (mask == 0) {
                        *dest++ = val;
                        val = 0;
                        mask = 0x80;
                    }
#endif /* USE_32_BIT_WRITES */
                    if (thresh_ptr >= thresh_limit)
                        thresh_ptr = thresh_line_base;
                } /* end src pixel loop - collect last bits if any */
                /* the following relies on their being enough 'pad' in dithered_line */
#ifdef USE_32_BIT_WRITES
                if (mask != &bit_order[0]) {
                    *dest = val;
                }
#else   /* example 8-bit code */
                if (mask != 0x80) {
                    *dest = val;
                }
#endif /* USE_32_BIT_WRITES */
#endif /* SKIP_HALFTONING_FOR_TIMING */
                TIFFWriteScanline(tfdev->tiff[comp_num], (tdata_t)dithered_line, y, 0);
            } /* end component loop */
        }
        /* Update the strip data */
        for (comp_num = 0; comp_num < num_comp; comp_num++ ) {
            TIFFWriteDirectory(tfdev->tiff[comp_num]);
            if (fmt) {
                int sep_num = map_comp_to_sep[comp_num];

                code = create_separation_file_name((tiffsep_device *)tfdev, name, gp_file_name_sizeof, sep_num, false);
                if (code < 0) {
                    code1 = code;
                    continue;
                }
                code = tiffsep_close_sep_file((tiffsep_device *)tfdev, name, comp_num);
                if (code >= 0)
                    code = gs_remove_outputfile_control_path(tfdev->memory, name);
                if (code < 0) {
                    code1 = code;
                }
            }
        }
        code = code1;

        /* free any allocations and exit with code */
cleanup:
        gs_free_object(pdev->memory, dithered_line, "tiffsep1_print_page");
        for (comp_num = 0; comp_num < num_comp; comp_num++) {
            gs_free_object(pdev->memory, planes[comp_num], "tiffsep1_print_page");
        }
    }
    /*
     * If we have any non encodable pixels then signal an error.
     */
    if (non_encodable_count) {
        dmlprintf1(pdev->memory, "WARNING:  Non encodable pixels = %d\n", non_encodable_count);
        code = gs_note_error(gs_error_rangecheck);
    }

done:
    if (name)
        gs_free_object(pdev->memory, name, "tiffsep1_print_page(name)");
    return code;
}