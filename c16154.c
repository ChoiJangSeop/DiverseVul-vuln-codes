static int display_preclose(void *handle, void *device)
{
    IMAGE *img = image_find(handle, device);
    if (img == NULL)
        return -1;

    gtk_main_iteration_do(FALSE);

    img->buf = NULL;
    img->width = 0;
    img->height = 0;
    img->rowstride = 0;
    img->format = 0;

    gtk_widget_destroy(img->window);
    img->window = NULL;
    img->scroll = NULL;
    img->darea = NULL;
    if (img->cmap)
        gdk_rgb_cmap_free(img->cmap);
    img->cmap = NULL;
    if (img->rgbbuf)
        free(img->rgbbuf);
    img->rgbbuf = NULL;

    gtk_main_iteration_do(FALSE);

    return 0;
}