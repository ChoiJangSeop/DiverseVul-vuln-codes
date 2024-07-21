c_pdf14trans_clist_read_update(gs_composite_t *	pcte, gx_device	* cdev,
                gx_device * tdev, gs_gstate * pgs, gs_memory_t * mem)
{
    pdf14_device * p14dev = (pdf14_device *)tdev;
    gs_pdf14trans_t * pdf14pct = (gs_pdf14trans_t *) pcte;
    gs_devn_params * pclist_devn_params;
    gx_device_clist_reader *pcrdev = (gx_device_clist_reader *)cdev;
    cmm_profile_t *cl_icc_profile, *p14_icc_profile;
    gsicc_rendering_param_t render_cond;
    cmm_dev_profile_t *dev_profile;

    dev_proc(cdev, get_profile)(cdev,  &dev_profile);
    gsicc_extract_profile(GS_UNKNOWN_TAG, dev_profile, &cl_icc_profile,
                          &render_cond);

    /* If we are using the blending color space, then be sure to use that. */
    if (p14dev->using_blend_cs && dev_profile->blend_profile != NULL)
        cl_icc_profile = dev_profile->blend_profile;

    dev_proc(p14dev, get_profile)((gx_device *)p14dev,  &dev_profile);
    gsicc_extract_profile(GS_UNKNOWN_TAG, dev_profile, &p14_icc_profile,
                          &render_cond);
    /*
     * We only handle the push/pop operations. Save and restore the color_info
     * field for the clist device.  This is needed since the process color
     * model of the clist device needs to match the PDF 1.4 compositing
     * device.
     */
    switch (pdf14pct->params.pdf14_op) {
        case PDF14_PUSH_DEVICE:
            gsicc_adjust_profile_rc(cl_icc_profile, 1, "c_pdf14trans_clist_read_update");
            gsicc_adjust_profile_rc(p14dev->icc_struct->device_profile[0], -1, "c_pdf14trans_clist_read_update");
            p14dev->icc_struct->device_profile[0] = cl_icc_profile;
            /*
             * If we are blending using spot colors (i.e. the output device
             * supports spot colors) then we need to transfer
             * color info from the clist PDF 1.4 compositing reader device
             * to the clist writer PDF 1.4 compositing device.
             * This info was transfered from that device to the output
             * device as a set of device parameters.  However the clist
             * reader PDF 1.4 compositing device did not exist when the
             * device parameters were read from the clist.  So that info
             * was buffered into the output device.
             */
            pclist_devn_params = dev_proc(cdev, ret_devn_params)(cdev);
            if (pclist_devn_params != NULL && pclist_devn_params->page_spot_colors != 0) {
                int num_comp = p14dev->color_info.num_components;
                /*
                 * The number of components for the PDF14 device is the sum
                 * of the process components and the number of spot colors
                 * for the page.  If the color capabilities of the parent
                 * device (which coming into this are the same as the p14dev)
                 * are smaller than the number of page spot colors then
                 * use that for the number of components. Otherwise use
                 * the page_spot_colors.
                 */
                p14dev->devn_params.page_spot_colors =
                    pclist_devn_params->page_spot_colors;
                if (num_comp < p14dev->devn_params.page_spot_colors + 4 ) {
                    p14dev->color_info.num_components = num_comp;
                } else {
                    /* if page_spot_colors < 0, this will be wrong, so don't update num_components */
                    if (p14dev->devn_params.page_spot_colors >= 0) {
                        p14dev->color_info.num_components =
                            p14dev->devn_params.num_std_colorant_names +
                            p14dev->devn_params.page_spot_colors;
                    }
                }
                /* limit the num_components to the max. */
                if (p14dev->color_info.num_components > p14dev->color_info.max_components)
                    p14dev->color_info.num_components = p14dev->color_info.max_components;
                /* Transfer the data for the spot color names
                   But we have to free what may be there before we do this */
                devn_free_params((gx_device*) p14dev);
                p14dev->devn_params.separations =
                    pclist_devn_params->pdf14_separations;
                p14dev->free_devicen = false;  /* to avoid freeing the clist ones */
                if (num_comp != p14dev->color_info.num_components) {
                    /* When the pdf14 device is opened it creates a context
                       and some soft mask related objects.  The push device
                       compositor action will have already created these but
                       they are the wrong size.  We must destroy them though
                       before reopening the device */
                    if (p14dev->ctx != NULL) {
                        pdf14_ctx_free(p14dev->ctx);
                    }
                    dev_proc(tdev, open_device) (tdev);
                }
            }
            /* Check if we need to swap out the ICC profile for the pdf14
               device.  This will occur if our source profile for our device
               happens to be something like CIELAB.  Then we will blend in
               RGB (unless a trans group is specified) */
            if (cl_icc_profile->data_cs == gsCIELAB || cl_icc_profile->islab) {
                gsicc_adjust_profile_rc(p14dev->icc_struct->device_profile[0], -1, "c_pdf14trans_clist_read_update");
                /* Initial ref count from gsicc_read_serial_icc() is 1, which is what we want */
                p14dev->icc_struct->device_profile[0] = gsicc_read_serial_icc(cdev, pcrdev->trans_dev_icc_hash);
                /* Keep a pointer to the clist device */
                p14dev->icc_struct->device_profile[0]->dev = (gx_device *) cdev;
            }
            break;

        case PDF14_POP_DEVICE:
#	    if 0 /* Disabled because *p14dev has no forwarding methods during
                    the clist playback. This code is not executed while clist
                    writing. */
            cdev->color_info = p14dev->saved_target_color_info;
#	    endif
            break;

        default:
            break;		/* do nothing for remaining ops */
    }

    return 0;
}