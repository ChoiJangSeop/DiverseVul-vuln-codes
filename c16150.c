static int display_open(void *handle, void *device)
{

    IMAGE *img = (IMAGE *)malloc(sizeof(IMAGE));
    if (img == NULL)
        return -1;
    memset(img, 0, sizeof(IMAGE));

    if (first_image == NULL) {
        gdk_rgb_init();
        gtk_widget_set_default_colormap(gdk_rgb_get_cmap());
        gtk_widget_set_default_visual(gdk_rgb_get_visual());
    }

    /* add to list */
    if (first_image)
        img->next = first_image;
    first_image = img;

    /* remember device and handle */
    img->handle = handle;
    img->device = device;

    /* create window */
    window_create(img);

    gtk_main_iteration_do(FALSE);
    return 0;
}