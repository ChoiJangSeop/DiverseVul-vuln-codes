static void window_resize(IMAGE *img)
{
    gtk_drawing_area_size(GTK_DRAWING_AREA (img->darea),
        img->width, img->height);
    if (!(GTK_WIDGET_FLAGS(img->window) & GTK_VISIBLE)) {
        /* We haven't yet shown the window, so set a default size
         * which is smaller than the desktop to allow room for
         * desktop toolbars, and if possible a little larger than
         * the image to allow room for the scroll bars.
         * We don't know the width of the scroll bars, so just guess. */
        gtk_window_set_default_size(GTK_WINDOW(img->window),
            min(gdk_screen_width()-96, img->width+24),
            min(gdk_screen_height()-96, img->height+24));
    }
}