set_color_procs(gx_device * pdev,
        dev_t_proc_encode_color((*encode_color), gx_device),
        dev_t_proc_decode_color((*decode_color), gx_device),
        dev_t_proc_get_color_mapping_procs((*get_color_mapping_procs), gx_device),
        dev_t_proc_get_color_comp_index((*get_color_comp_index), gx_device))
{
#if 0				/* These procs are no longer used */
    pdev->procs.map_rgb_color = encode_color;
    pdev->procs.map_color_rgb = decode_color;
#endif
    pdev->procs.get_color_mapping_procs = get_color_mapping_procs;
    pdev->procs.get_color_comp_index = get_color_comp_index;
    pdev->procs.encode_color = encode_color;
    pdev->procs.decode_color = decode_color;
}