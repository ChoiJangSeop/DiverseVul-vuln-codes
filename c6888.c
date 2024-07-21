zimage3(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_image3_t image;
    int interleave_type;
    ref *pDataDict;
    ref *pMaskDict;
    image_params ip_data, ip_mask;
    int ignored;
    int code, mcode;

    check_type(*op, t_dictionary);
    check_dict_read(*op);
    if ((code = dict_int_param(op, "InterleaveType", 1, 3, -1,
                               &interleave_type)) < 0
        )
        return code;
    gs_image3_t_init(&image, NULL, interleave_type);
    if (dict_find_string(op, "DataDict", &pDataDict) <= 0 ||
        dict_find_string(op, "MaskDict", &pMaskDict) <= 0
        )
        return_error(gs_error_rangecheck);
    if ((code = pixel_image_params(i_ctx_p, pDataDict,
                        (gs_pixel_image_t *)&image, &ip_data,
                        12, false, gs_currentcolorspace(igs))) < 0 ||
        (mcode = code = data_image_params(imemory, pMaskDict, &image.MaskDict,
                                   &ip_mask, false, 1, 12, false, false)) < 0 ||
        (code = dict_int_param(pDataDict, "ImageType", 1, 1, 0, &ignored)) < 0 ||
        (code = dict_int_param(pMaskDict, "ImageType", 1, 1, 0, &ignored)) < 0
        )
        return code;
    /*
     * MaskDict must have a DataSource iff InterleaveType == 3.
     */
    if ((ip_data.MultipleDataSources && interleave_type != 3) ||
        ip_mask.MultipleDataSources ||
        mcode != (image.InterleaveType != 3)
        )
        return_error(gs_error_rangecheck);
    if (image.InterleaveType == 3) {
        /* Insert the mask DataSource before the data DataSources. */
        memmove(&ip_data.DataSource[1], &ip_data.DataSource[0],
                (countof(ip_data.DataSource) - 1) *
                sizeof(ip_data.DataSource[0]));
        ip_data.DataSource[0] = ip_mask.DataSource[0];
    }
    /* We never interpolate images with masks */
    image.Interpolate = 0;
    return zimage_setup(i_ctx_p, (gs_pixel_image_t *)&image,
                        &ip_data.DataSource[0],
                        image.CombineWithColor, 1);
}