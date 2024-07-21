set_rgb_color_procs(gx_device * pdev,
        dev_t_proc_encode_color((*encode_color), gx_device),
        dev_t_proc_decode_color((*decode_color), gx_device))
{
    set_color_procs(pdev, encode_color, decode_color,
        gx_default_DevRGB_get_color_mapping_procs,
        gx_default_DevRGB_get_color_comp_index);
}