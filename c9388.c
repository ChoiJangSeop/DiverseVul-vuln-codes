xps_image_end_image(gx_image_enum_common_t * info, bool draw_last)
{
    xps_image_enum_t *pie = (xps_image_enum_t *)info;
    int code = 0;

    /* N.B. Write the final strip, if any. */

    code = TIFFWriteDirectory(pie->tif);
    TIFFCleanup(pie->tif);

    /* Stuff the image into the zip archive and close the file */
    code = xps_add_tiff_image(pie);
    if (code < 0)
        goto exit;

    /* Reset the brush type to solid */
    xps_setstrokebrush((gx_device_xps *) (pie->dev), xps_solidbrush);
    xps_setfillbrush((gx_device_xps *) (pie->dev), xps_solidbrush);

    /* Add the image relationship */
    code = xps_add_image_relationship(pie);

exit:
    if (pie->pcs != NULL)
        rc_decrement(pie->pcs, "xps_image_end_image (pcs)");
    if (pie->buffer != NULL)
        gs_free_object(pie->memory, pie->buffer, "xps_image_end_image");
    if (pie->devc_buffer != NULL)
        gs_free_object(pie->memory, pie->devc_buffer, "xps_image_end_image");

    /* ICC clean up */
    if (pie->icc_link != NULL)
        gsicc_release_link(pie->icc_link);

    return code;
}