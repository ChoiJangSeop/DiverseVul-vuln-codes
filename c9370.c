tiffsep_print_page(gx_device_printer * pdev, gp_file * file)
{
    tiffsep_device * const tfdev = (tiffsep_device *)pdev;
    int num_std_colorants = tfdev->devn_params.num_std_colorant_names;
    int num_order = tfdev->devn_params.num_separation_order_names;
    int num_spot = tfdev->devn_params.separations.num_separations;
    int num_comp, comp_num, sep_num, code = 0, code1 = 0;
    cmyk_composite_map cmyk_map[GX_DEVICE_COLOR_MAX_COMPONENTS];
    char *name = NULL;
    bool double_f = false;
    int base_filename_length = length_base_file_name(tfdev, &double_f);
    int save_depth = pdev->color_info.depth;
    int save_numcomps = pdev->color_info.num_components;
    const char *fmt;
    gs_parsed_file_name_t parsed;
    int plane_count = 0;  /* quiet compiler */
    int factor = tfdev->downscale.downscale_factor;
    int mfs = tfdev->downscale.min_feature_size;
    int dst_bpc = tfdev->BitsPerComponent;
    gx_downscaler_t ds;
    int width = gx_downscaler_scale(tfdev->width, factor);
    int height = gx_downscaler_scale(tfdev->height, factor);

    name = (char *)gs_alloc_bytes(pdev->memory, gp_file_name_sizeof, "tiffsep_print_page(name)");
    if (!name)
        return_error(gs_error_VMerror);

    /* Print the names of the spot colors */
    if (num_order == 0) {
        for (sep_num = 0; sep_num < num_spot; sep_num++) {
            copy_separation_name(tfdev, name,
                gp_file_name_sizeof - base_filename_length - SUFFIX_SIZE, sep_num, 0);
            dmlprintf1(pdev->memory, "%%%%SeparationName: %s\n", name);
        }
    }

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
                "\nUse of the %%d format is required to output more than one page to tiffsep.\n"
                "See doc/Devices.htm#TIFF for details.\n\n");
       code = gs_note_error(gs_error_ioerror);
    }
    /* Write the page directory for the CMYK equivalent file. */
    if (!tfdev->comp_file) {
        pdev->color_info.depth = dst_bpc*4;        /* Create directory for 32 bit cmyk */
        if (!tfdev->UseBigTIFF && tfdev->Compression==COMPRESSION_NONE &&
            height > ((unsigned long) 0xFFFFFFFF - (file ? gp_ftell(file) : 0))/(width*4)) { /* note width is never 0 in print_page */
            dmprintf(pdev->memory, "CMYK composite file would be too large! Reduce resolution or enable compression.\n");
            return_error(gs_error_rangecheck);  /* this will overflow 32 bits */
        }

        code = gx_device_open_output_file((gx_device *)pdev, pdev->fname, true, true, &(tfdev->comp_file));
        if (code < 0) {
            goto done;
        }

        tfdev->tiff_comp = tiff_from_filep(pdev, pdev->dname, tfdev->comp_file, tfdev->BigEndian, tfdev->UseBigTIFF);
        if (!tfdev->tiff_comp) {
            code = gs_note_error(gs_error_invalidfileaccess);
            goto done;
        }

    }
    code = tiff_set_fields_for_printer(pdev, tfdev->tiff_comp, factor, 0, tfdev->write_datetime);

    if (dst_bpc == 1 || dst_bpc == 8) {
        tiff_set_cmyk_fields(pdev, tfdev->tiff_comp, dst_bpc, tfdev->Compression, tfdev->MaxStripSize);
    }
    else {
        /* Catch-all just for safety's sake */
        tiff_set_cmyk_fields(pdev, tfdev->tiff_comp, dst_bpc, COMPRESSION_NONE, tfdev->MaxStripSize);
    }

    pdev->color_info.depth = save_depth;
    if (code < 0) {
        goto done;
    }

    /* Set up the separation output files */
    num_comp = number_output_separations( tfdev->color_info.num_components,
                                        num_std_colorants, num_order, num_spot);

    if (!tfdev->NoSeparationFiles && !num_order && num_comp < num_std_colorants + num_spot) {
        dmlprintf(pdev->memory, "Warning: skipping one or more colour separations, see: Devices.htm#TIFF\n");
    }

    if (!tfdev->NoSeparationFiles) {
        for (comp_num = 0; comp_num < num_comp; comp_num++) {
            int sep_num = tfdev->devn_params.separation_order_map[comp_num];

            code = create_separation_file_name(tfdev, name, gp_file_name_sizeof,
                sep_num, true);
            if (code < 0) {
                goto done;
            }

            /*
             * Close the old separation file if we are creating individual files
             * for each page.
             */
            if (tfdev->sep_file[comp_num] != NULL && fmt != NULL) {
                code = tiffsep_close_sep_file(tfdev, name, comp_num);
                if (code >= 0)
                    code = gs_remove_outputfile_control_path(tfdev->memory, name);
                if (code < 0)
                    return code;
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

            pdev->color_info.depth = dst_bpc;     /* Create files for 8 bit gray */
            pdev->color_info.num_components = 1;
            if (!tfdev->UseBigTIFF && tfdev->Compression == COMPRESSION_NONE &&
                height * 8 / dst_bpc > ((unsigned long)0xFFFFFFFF - (file ? gp_ftell(file) : 0)) / width) /* note width is never 0 in print_page */
            {
                code = gs_note_error(gs_error_rangecheck);  /* this will overflow 32 bits */
                goto done;
            }


            code = tiff_set_fields_for_printer(pdev, tfdev->tiff[comp_num], factor, 0, tfdev->write_datetime);
            tiff_set_gray_fields(pdev, tfdev->tiff[comp_num], dst_bpc, tfdev->Compression, tfdev->MaxStripSize);
            pdev->color_info.depth = save_depth;
            pdev->color_info.num_components = save_numcomps;
            if (code < 0) {
                goto done;
            }
        }
    }

    build_cmyk_map((gx_device*) tfdev, num_comp, &tfdev->equiv_cmyk_colors, cmyk_map);
    if (tfdev->PrintSpotCMYK) {
        code = print_cmyk_equivalent_colors(tfdev, num_comp, cmyk_map);
        if (code < 0) {
            goto done;
        }
    }

    {
        int raster_plane = bitmap_raster(width * 8);
        byte *planes[GS_CLIENT_COLOR_MAX_COMPONENTS] = { 0 };
        int cmyk_raster = width * NUM_CMYK_COMPONENTS;
        int pixel, y;
        byte * sep_line;
        int plane_index;
        int offset_plane = 0;

        sep_line =
            gs_alloc_bytes(pdev->memory, cmyk_raster, "tiffsep_print_page");
        if (!sep_line) {
            code = gs_note_error(gs_error_VMerror);
            goto done;
        }

        if (!tfdev->NoSeparationFiles)
            for (comp_num = 0; comp_num < num_comp; comp_num++ )
                TIFFCheckpointDirectory(tfdev->tiff[comp_num]);
        TIFFCheckpointDirectory(tfdev->tiff_comp);

        /* Write the page data. */
        {
            gs_get_bits_params_t params;
            int byte_width;

            /* Return planar data */
            params.options = (GB_RETURN_POINTER | GB_RETURN_COPY |
                 GB_ALIGN_STANDARD | GB_OFFSET_0 | GB_RASTER_STANDARD |
                 GB_PACKING_PLANAR | GB_COLORS_NATIVE | GB_ALPHA_NONE);
            params.x_offset = 0;
            params.raster = bitmap_raster(width * pdev->color_info.depth);

            if (num_order > 0) {
                /* In this case, there was a specification for a separation
                   color order, which indicates what colorants we will
                   actually creat individual separation files for.  We need
                   to allocate for the standard colorants.  This is due to the
                   fact that even when we specify a single spot colorant, we
                   still create the composite CMYK output file. */
                for (comp_num = 0; comp_num < num_std_colorants; comp_num++) {
                    planes[comp_num] = gs_alloc_bytes(pdev->memory, raster_plane,
                                                      "tiffsep_print_page");
                    params.data[comp_num] = planes[comp_num];
                    if (params.data[comp_num] == NULL) {
                        code = gs_note_error(gs_error_VMerror);
                        goto cleanup;
                    }
                }
                offset_plane = num_std_colorants;
                /* Now we need to make sure that we do not allocate extra
                   planes if any of the colorants in the order list are
                   one of the standard colorant names */
                plane_index = plane_count = num_std_colorants;
                for (comp_num = 0; comp_num < num_comp; comp_num++) {
                    int temp_pos;

                    temp_pos = tfdev->devn_params.separation_order_map[comp_num];
                    if (temp_pos >= num_std_colorants) {
                        /* We have one that is not a standard colorant name
                           so allocate a new plane */
                        planes[plane_count] = gs_alloc_bytes(pdev->memory, raster_plane,
                                                        "tiffsep_print_page");
                        /* Assign the new plane to the appropriate position */
                        params.data[plane_index] = planes[plane_count];
                        if (params.data[plane_index] == NULL) {
                            code = gs_note_error(gs_error_VMerror);
                            goto cleanup;
                        }
                        plane_count += 1;
                    } else {
                        /* Assign params.data with the appropriate std.
                           colorant plane position */
                        params.data[plane_index] = planes[temp_pos];
                    }
                    plane_index += 1;
                }
            } else {
                /* Sep color order number was not specified so just render all
                   the  planes that we can */
                for (comp_num = 0; comp_num < num_comp; comp_num++) {
                    planes[comp_num] = gs_alloc_bytes(pdev->memory, raster_plane,
                                                    "tiffsep_print_page");
                    params.data[comp_num] = planes[comp_num];
                    if (params.data[comp_num] == NULL) {
                        code = gs_note_error(gs_error_VMerror);
                        goto cleanup;
                    }
                }
            }
            code = gx_downscaler_init_planar_trapped(&ds, (gx_device *)pdev, &params,
                                                     num_comp, factor, mfs, 8, dst_bpc,
                                                     tfdev->downscale.trap_w, tfdev->downscale.trap_h,
                                                     tfdev->downscale.trap_order);
            if (code < 0)
                goto cleanup;
            byte_width = (width * dst_bpc + 7)>>3;
            for (y = 0; y < height; ++y) {
                code = gx_downscaler_get_bits_rectangle(&ds, &params, y);
                if (code < 0)
                    goto cleanup;
                /* Write separation data (tiffgray format) */
                if (!tfdev->NoSeparationFiles) {
                    for (comp_num = 0; comp_num < num_comp; comp_num++) {
                        byte *src;
                        byte *dest = sep_line;

                        if (num_order > 0) {
                            src = params.data[tfdev->devn_params.separation_order_map[comp_num]];
                        }
                        else
                            src = params.data[comp_num];
                        for (pixel = 0; pixel < byte_width; pixel++, dest++, src++)
                            *dest = MAX_COLOR_VALUE - *src;    /* Gray is additive */
                        TIFFWriteScanline(tfdev->tiff[comp_num], (tdata_t)sep_line, y, 0);
                    }
                }
                /* Write CMYK equivalent data */
                switch(dst_bpc)
                {
                default:
                case 8:
                    build_cmyk_raster_line_fromplanar(&params, sep_line, width,
                                                      num_comp, cmyk_map, num_order,
                                                      tfdev);
                    break;
                case 4:
                    build_cmyk_raster_line_fromplanar_4bpc(&params, sep_line, width,
                                                           num_comp, cmyk_map, num_order,
                                                           tfdev);
                    break;
                case 2:
                    build_cmyk_raster_line_fromplanar_2bpc(&params, sep_line, width,
                                                           num_comp, cmyk_map, num_order,
                                                           tfdev);
                    break;
                case 1:
                    build_cmyk_raster_line_fromplanar_1bpc(&params, sep_line, width,
                                                           num_comp, cmyk_map, num_order,
                                                           tfdev);
                    break;
                }
                TIFFWriteScanline(tfdev->tiff_comp, (tdata_t)sep_line, y, 0);
            }
cleanup:
            if (num_order > 0) {
                /* Free up the standard colorants if num_order was set.
                   In this process, we need to make sure that none of them
                   were the standard colorants.  plane_count should have
                   the sum of the std. colorants plus any non-standard
                   ones listed in separation color order */
                for (comp_num = 0; comp_num < plane_count; comp_num++) {
                    gs_free_object(pdev->memory, planes[comp_num],
                                                    "tiffsep_print_page");
                }
            } else {
                for (comp_num = 0; comp_num < num_comp; comp_num++) {
                    gs_free_object(pdev->memory, planes[comp_num + offset_plane],
                                                    "tiffsep_print_page");
                }
            }
            gx_downscaler_fin(&ds);
            gs_free_object(pdev->memory, sep_line, "tiffsep_print_page");
        }
        code1 = code;
        if (!tfdev->NoSeparationFiles) {
            for (comp_num = 0; comp_num < num_comp; comp_num++) {
                TIFFWriteDirectory(tfdev->tiff[comp_num]);
                if (fmt) {
                    int sep_num = tfdev->devn_params.separation_order_map[comp_num];

                    code = create_separation_file_name(tfdev, name, gp_file_name_sizeof, sep_num, false);
                    if (code < 0) {
                        code1 = code;
                        continue;
                    }
                    code = tiffsep_close_sep_file(tfdev, name, comp_num);
                    if (code >= 0)
                        code = gs_remove_outputfile_control_path(tfdev->memory, name);
                    if (code < 0) {
                        code1 = code;
                    }
                }
            }
        }
        TIFFWriteDirectory(tfdev->tiff_comp);
        if (fmt) {
            code = tiffsep_close_comp_file(tfdev, pdev->fname);
        }
        if (code1 < 0) {
            code = code1;
        }
    }

done:
    if (name)
        gs_free_object(pdev->memory, name, "tiffsep_print_page(name)");
    return code;
}