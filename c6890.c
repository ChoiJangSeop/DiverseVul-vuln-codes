zimage3x(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_image3x_t image;
    ref *pDataDict;
    image_params ip_data;
    int num_components =
        gs_color_space_num_components(gs_currentcolorspace(igs));
    int ignored;
    int code;

    check_type(*op, t_dictionary);
    check_dict_read(*op);
    memset(&image, 0, sizeof(gs_image3x_t));
    gs_image3x_t_init(&image, NULL);
    if (dict_find_string(op, "DataDict", &pDataDict) <= 0)
        return_error(gs_error_rangecheck);
    if ((code = pixel_image_params(i_ctx_p, pDataDict,
                   (gs_pixel_image_t *)&image, &ip_data,
                   16, false, gs_currentcolorspace(igs))) < 0 ||
        (code = dict_int_param(pDataDict, "ImageType", 1, 1, 0, &ignored)) < 0
        )
        return code;
    /*
     * We have to process the masks in the reverse order, because they
     * insert their DataSource before the one(s) for the DataDict.
     */
    if ((code = mask_dict_param(imemory, op, &ip_data,
                                "ShapeMaskDict", num_components,
                                &image.Shape)) < 0 ||
        (code = mask_dict_param(imemory, op, &ip_data,
                                "OpacityMaskDict", num_components,
                                &image.Opacity)) < 0
        )
        return code;
    return zimage_setup(i_ctx_p, (gs_pixel_image_t *)&image,
                        &ip_data.DataSource[0],
                        image.CombineWithColor, 1);
}