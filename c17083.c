display_set_color_format(gx_device_display *ddev, int nFormat)
{
    gx_device * pdev = (gx_device *) ddev;
    gx_device_color_info dci = ddev->color_info;
    int bpc;	/* bits per component */
    int bpp;	/* bits per pixel */
    int maxvalue;
    int align;

    switch (nFormat & DISPLAY_DEPTH_MASK) {
        case DISPLAY_DEPTH_1:
            bpc = 1;
            break;
        case DISPLAY_DEPTH_2:
            bpc = 2;
            break;
        case DISPLAY_DEPTH_4:
            bpc = 4;
            break;
        case DISPLAY_DEPTH_8:
            bpc = 8;
            break;
        case DISPLAY_DEPTH_12:
            bpc = 12;
            break;
        case DISPLAY_DEPTH_16:
            bpc = 16;
            break;
        default:
            return_error(gs_error_rangecheck);
    }
    maxvalue = (1 << bpc) - 1;
    ddev->devn_params.bitspercomponent = bpc;

    switch (ddev->nFormat & DISPLAY_ROW_ALIGN_MASK) {
        case DISPLAY_ROW_ALIGN_DEFAULT:
            align = ARCH_ALIGN_PTR_MOD;
            break;
        case DISPLAY_ROW_ALIGN_4:
            align = 4;
            break;
        case DISPLAY_ROW_ALIGN_8:
            align = 8;
            break;
        case DISPLAY_ROW_ALIGN_16:
            align = 16;
            break;
        case DISPLAY_ROW_ALIGN_32:
            align = 32;
            break;
        case DISPLAY_ROW_ALIGN_64:
            align = 64;
            break;
        default:
            align = 0;	/* not permitted */
    }
    if (align < ARCH_ALIGN_PTR_MOD)
        return_error(gs_error_rangecheck);

    switch (ddev->nFormat & DISPLAY_ALPHA_MASK) {
        case DISPLAY_ALPHA_FIRST:
        case DISPLAY_ALPHA_LAST:
            /* Not implemented and unlikely to ever be implemented
             * because they would interact with linear_and_separable
             */
            return_error(gs_error_rangecheck);
    }

    switch (nFormat & DISPLAY_COLORS_MASK) {
        case DISPLAY_COLORS_NATIVE:
            switch (nFormat & DISPLAY_DEPTH_MASK) {
                case DISPLAY_DEPTH_1:
                    /* 1bit/pixel, black is 1, white is 0 */
                    set_color_info(&dci, DISPLAY_MODEL_GRAY, 1, 1, 1, 0);
                    dci.separable_and_linear = GX_CINFO_SEP_LIN_NONE;
                    set_gray_color_procs(pdev, gx_b_w_gray_encode,
                                                gx_default_b_w_map_color_rgb);
                    break;
                case DISPLAY_DEPTH_4:
                    /* 4bit/pixel VGA color */
                    set_color_info(&dci, DISPLAY_MODEL_RGB, 3, 4, 3, 2);
                    dci.separable_and_linear = GX_CINFO_SEP_LIN_NONE;
                    set_rgb_color_procs(pdev, display_map_rgb_color_device4,
                                                display_map_color_rgb_device4);
                    break;
                case DISPLAY_DEPTH_8:
                    /* 8bit/pixel 96 color palette */
                    set_color_info(&dci, DISPLAY_MODEL_RGBK, 4, 8, 31, 3);
                    dci.separable_and_linear = GX_CINFO_SEP_LIN_NONE;
                    set_rgbk_color_procs(pdev, display_encode_color_device8,
                                                display_decode_color_device8);
                    break;
                case DISPLAY_DEPTH_16:
                    /* Windows 16-bit display */
                    /* Is maxgray = maxcolor = 63 correct? */
                    if ((ddev->nFormat & DISPLAY_555_MASK)
                        == DISPLAY_NATIVE_555)
                        set_color_info(&dci, DISPLAY_MODEL_RGB, 3, 16, 31, 31);
                    else
                        set_color_info(&dci, DISPLAY_MODEL_RGB, 3, 16, 63, 63);
                    set_rgb_color_procs(pdev, display_map_rgb_color_device16,
                                                display_map_color_rgb_device16);
                    break;
                default:
                    return_error(gs_error_rangecheck);
            }
            dci.gray_index = GX_CINFO_COMP_NO_INDEX;
            break;
        case DISPLAY_COLORS_GRAY:
            set_color_info(&dci, DISPLAY_MODEL_GRAY, 1, bpc, maxvalue, 0);
            if (bpc == 1)
                set_gray_color_procs(pdev, gx_default_gray_encode,
                                                gx_default_w_b_map_color_rgb);
            else
                set_gray_color_procs(pdev, gx_default_gray_encode,
                                                gx_default_gray_map_color_rgb);
            break;
        case DISPLAY_COLORS_RGB:
            if ((nFormat & DISPLAY_ALPHA_MASK) == DISPLAY_ALPHA_NONE)
                bpp = bpc * 3;
            else
                bpp = bpc * 4;
            set_color_info(&dci, DISPLAY_MODEL_RGB, 3, bpp, maxvalue, maxvalue);
            if (((nFormat & DISPLAY_DEPTH_MASK) == DISPLAY_DEPTH_8) &&
                ((nFormat & DISPLAY_ALPHA_MASK) == DISPLAY_ALPHA_NONE)) {
                if ((nFormat & DISPLAY_ENDIAN_MASK) == DISPLAY_BIGENDIAN)
                    set_rgb_color_procs(pdev, gx_default_rgb_map_rgb_color,
                                                gx_default_rgb_map_color_rgb);
                else
                    set_rgb_color_procs(pdev, display_map_rgb_color_bgr24,
                                                display_map_color_rgb_bgr24);
            }
            else {
                /* slower flexible functions for alpha/unused component */
                set_rgb_color_procs(pdev, display_map_rgb_color_rgb,
                                                display_map_color_rgb_rgb);
            }
            break;
        case DISPLAY_COLORS_CMYK:
            bpp = bpc * 4;
            set_color_info(&dci, DISPLAY_MODEL_CMYK, 4, bpp, maxvalue, maxvalue);
            if ((nFormat & DISPLAY_ALPHA_MASK) != DISPLAY_ALPHA_NONE)
                return_error(gs_error_rangecheck);
            if ((nFormat & DISPLAY_ENDIAN_MASK) != DISPLAY_BIGENDIAN)
                return_error(gs_error_rangecheck);

            if ((nFormat & DISPLAY_DEPTH_MASK) == DISPLAY_DEPTH_1)
                set_cmyk_color_procs(pdev, cmyk_1bit_map_cmyk_color,
                                                cmyk_1bit_map_color_cmyk);
            else if ((nFormat & DISPLAY_DEPTH_MASK) == DISPLAY_DEPTH_8)
                set_cmyk_color_procs(pdev, cmyk_8bit_map_cmyk_color,
                                                cmyk_8bit_map_color_cmyk);
            else
                return_error(gs_error_rangecheck);
            break;
        case DISPLAY_COLORS_SEPARATION:
            if ((nFormat & DISPLAY_ENDIAN_MASK) != DISPLAY_BIGENDIAN)
                return_error(gs_error_rangecheck);
            bpp = ARCH_SIZEOF_COLOR_INDEX * 8;
            set_color_info(&dci, DISPLAY_MODEL_SEP, bpp/bpc, bpp,
                maxvalue, maxvalue);
            if ((nFormat & DISPLAY_DEPTH_MASK) == DISPLAY_DEPTH_8) {
                ddev->devn_params.bitspercomponent = bpc;
                set_color_procs(pdev,
                    display_separation_encode_color,
                    display_separation_decode_color,
                    display_separation_get_color_mapping_procs,
                    display_separation_get_color_comp_index);
            }
            else
                return_error(gs_error_rangecheck);
            break;
        default:
            return_error(gs_error_rangecheck);
    }

    /* restore old anti_alias info */
    dci.anti_alias = ddev->color_info.anti_alias;
    ddev->color_info = dci;
    check_device_separable(pdev);
    switch (nFormat & DISPLAY_COLORS_MASK) {
        case DISPLAY_COLORS_NATIVE:
            ddev->color_info.gray_index = GX_CINFO_COMP_NO_INDEX;
            if ((nFormat & DISPLAY_DEPTH_MASK) == DISPLAY_DEPTH_1)
                ddev->color_info.gray_index = 0;
            else if ((nFormat & DISPLAY_DEPTH_MASK) == DISPLAY_DEPTH_8)
                ddev->color_info.gray_index = 3;
            break;
        case DISPLAY_COLORS_RGB:
            ddev->color_info.gray_index = GX_CINFO_COMP_NO_INDEX;
            break;
        case DISPLAY_COLORS_GRAY:
            ddev->color_info.gray_index = 0;
            break;
        case DISPLAY_COLORS_CMYK:
            ddev->color_info.gray_index = 3;
            break;
        case DISPLAY_COLORS_SEPARATION:
            ddev->color_info.gray_index = GX_CINFO_COMP_NO_INDEX;
            break;
    }
    ddev->nFormat = nFormat;

    return 0;
}