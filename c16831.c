gx_begin_image1(gx_device * dev,
                const gs_gstate * pgs, const gs_matrix * pmat,
                const gs_image_common_t * pic, const gs_int_rect * prect,
                const gx_drawing_color * pdcolor, const gx_clip_path * pcpath,
                gs_memory_t * mem, gx_image_enum_common_t ** pinfo)
{
    gx_image_enum *penum;
    const gs_image_t *pim = (const gs_image_t *)pic;
    int code = gx_image_enum_alloc(pic, prect, mem, &penum);

    if (code < 0)
        return code;
    memset(penum, 0, sizeof(gx_image_enum));	/* in case of failure, no dangling pointers */
    penum->alpha = pim->Alpha;
    penum->use_mask_color = false;
    penum->image_parent_type = pim->image_parent_type;
    penum->masked = pim->ImageMask;
    penum->adjust =
        (pim->ImageMask && pim->adjust ? float2fixed(0.25) : fixed_0);
    code = gx_image_enum_begin(dev, pgs, pmat, pic, pdcolor, pcpath, mem,
                               penum);
    if (code >= 0)
        *pinfo = (gx_image_enum_common_t *)penum;
    return code;
}