pdf14_open(gx_device *dev)
{
    pdf14_device *pdev = (pdf14_device *)dev;
    gs_int_rect rect;

    if_debug2m('v', dev->memory, "[v]pdf14_open: width = %d, height = %d\n",
               dev->width, dev->height);
    rect.p.x = 0;
    rect.p.y = 0;
    rect.q.x = dev->width;
    rect.q.y = dev->height;
    pdev->ctx = pdf14_ctx_new(&rect, dev->color_info.num_components,
        pdev->color_info.polarity != GX_CINFO_POLARITY_SUBTRACTIVE, dev);
    if (pdev->ctx == NULL)
        return_error(gs_error_VMerror);
    pdev->free_devicen = true;
    return 0;
}