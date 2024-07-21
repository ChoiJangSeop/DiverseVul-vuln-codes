mask_dict_param(const gs_memory_t *mem, os_ptr op,
image_params *pip_data, const char *dict_name,
                int num_components, gs_image3x_mask_t *pixm)
{
    ref *pMaskDict;
    image_params ip_mask;
    int ignored;
    int code, mcode;

    if (dict_find_string(op, dict_name, &pMaskDict) <= 0)
        return 1;
    if ((mcode = code = data_image_params(mem, pMaskDict, &pixm->MaskDict,
                                          &ip_mask, false, 1, 16, false, false)) < 0 ||
        (code = dict_int_param(pMaskDict, "ImageType", 1, 1, 0, &ignored)) < 0 ||
        (code = dict_int_param(pMaskDict, "InterleaveType", 1, 3, -1,
                               &pixm->InterleaveType)) < 0 ||
        (code = dict_floats_param(mem, op, "Matte", num_components,
                                  pixm->Matte, NULL)) < 0
        )
        return code;
    pixm->has_Matte = code > 0;
    /*
     * The MaskDict must have a DataSource iff InterleaveType == 3.
     */
    if ((pip_data->MultipleDataSources && pixm->InterleaveType != 3) ||
        ip_mask.MultipleDataSources ||
        mcode != (pixm->InterleaveType != 3)
        )
        return_error(gs_error_rangecheck);
    if (pixm->InterleaveType == 3) {
        /* Insert the mask DataSource before the data DataSources. */
        memmove(&pip_data->DataSource[1], &pip_data->DataSource[0],
                (countof(pip_data->DataSource) - 1) *
                sizeof(pip_data->DataSource[0]));
        pip_data->DataSource[0] = ip_mask.DataSource[0];
    }
    return 0;
}