xps_finish_image_path(gx_device_vector *vdev)
{
    gx_device_xps *xps = (gx_device_xps *)vdev;
    char line[300];
    const char *fmt;
    gs_matrix matrix;

    /* Path is started.  Do the image brush image brush and close the path */
    write_str_to_current_page(xps, "\t<Path.Fill>\n");
    write_str_to_current_page(xps, "\t\t<ImageBrush ");
    fmt = "ImageSource = \"{ColorConvertedBitmap /%s /%s}\" Viewbox=\"%d, %d, %d, %d\" ViewboxUnits = \"Absolute\" Viewport = \"%d, %d, %d, %d\" ViewportUnits = \"Absolute\" TileMode = \"None\" >\n";
    gs_sprintf(line, fmt, xps->xps_pie->file_name, xps->xps_pie->icc_name,
        0, 0, xps->xps_pie->width, xps->xps_pie->height, 0, 0,
        xps->xps_pie->width, xps->xps_pie->height);
    write_str_to_current_page(xps, line);

    /* Now the render transform.  This is applied to the image brush. Path
    is already transformed */
    write_str_to_current_page(xps, "\t\t\t<ImageBrush.Transform>\n");
    fmt = "\t\t\t\t<MatrixTransform Matrix = \"%g,%g,%g,%g,%g,%g\" />\n";
    matrix = xps->xps_pie->mat;
    gs_sprintf(line, fmt,
        matrix.xx, matrix.xy, matrix.yx, matrix.yy, matrix.tx, matrix.ty);
    write_str_to_current_page(xps, line);
    write_str_to_current_page(xps, "\t\t\t</ImageBrush.Transform>\n");
    write_str_to_current_page(xps, "\t\t</ImageBrush>\n");
    write_str_to_current_page(xps, "\t</Path.Fill>\n");
    /* End this path */
    write_str_to_current_page(xps, "</Path>\n");
}