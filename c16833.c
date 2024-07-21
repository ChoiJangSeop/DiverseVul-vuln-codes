gx_begin_image4(gx_device * dev,
                const gs_gstate * pgs, const gs_matrix * pmat,
                const gs_image_common_t * pic, const gs_int_rect * prect,
                const gx_drawing_color * pdcolor, const gx_clip_path * pcpath,
                gs_memory_t * mem, gx_image_enum_common_t ** pinfo)
{
    gx_image_enum *penum;
    const gs_image4_t *pim = (const gs_image4_t *)pic;
    int code = gx_image_enum_alloc(pic, prect, mem, &penum);

    if (code < 0)
        return code;
    penum->alpha = gs_image_alpha_none;
    penum->masked = false;
    penum->adjust = fixed_0;
    penum->image_parent_type = gs_image_type4;
    /* Check that MaskColor values are within the valid range. */
    {
        bool opaque = false;
        uint max_value = (1 << pim->BitsPerComponent) - 1;
        int spp = cs_num_components(pim->ColorSpace);
        int i;

        for (i = 0; i < spp * 2; i += 2) {
            uint c0, c1;

            if (pim->MaskColor_is_range)
                c0 = pim->MaskColor[i], c1 = pim->MaskColor[i + 1];
            else
                c0 = c1 = pim->MaskColor[i >> 1];

            if ((c0 | c1) > max_value) {
                gs_free_object(mem, penum, "gx_begin_image4");
                return_error(gs_error_rangecheck);
            }
            if (c0 > c1) {
                opaque = true;	/* pixel can never match mask color */
                break;
            }
            penum->mask_color.values[i] = c0;
            penum->mask_color.values[i + 1] = c1;
        }
        penum->use_mask_color = !opaque;
    }
    code = gx_image_enum_begin(dev, pgs, pmat, pic, pdcolor, pcpath, mem,
                               penum);
    if (code >= 0)
        *pinfo = (gx_image_enum_common_t *)penum;
    return code;
}