window_draw(GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
    IMAGE *img = (IMAGE *)user_data;
    if (img && img->window && img->buf) {
        int color = img->format & DISPLAY_COLORS_MASK;
        int depth = img->format & DISPLAY_DEPTH_MASK;
        int x, y, width, height;
        if (event->area.x + event->area.width > img->width) {
            x = img->width;
            width = (event->area.x + event->area.width) - x;
            y = event->area.y;
            height = min(img->height, event->area.y + event->area.height) - y;
            gdk_window_clear_area(widget->window, x, y, width, height);
        }
        if (event->area.y + event->area.height > img->height) {
            x = event->area.x;
            width = event->area.width;
            y = img->height;
            height = (event->area.y + event->area.height) - y;
            gdk_window_clear_area(widget->window, x, y, width, height);
        }
        x = event->area.x;
        y = event->area.y;
        width = event->area.width;
        height = event->area.height;
        if ((x>=0) && (y>=0) && (x < img->width) && (y < img->height)) {
            /* drawing area intersects bitmap */
            if (x + width > img->width)
                width = img->width - x;
            if (y + height > img->height)
                height =  img->height - y;
            switch (color) {
                case DISPLAY_COLORS_NATIVE:
                    if (depth == DISPLAY_DEPTH_8)
                        gdk_draw_indexed_image(widget->window,
                            widget->style->fg_gc[GTK_STATE_NORMAL],
                            x, y, width, height,
                            GDK_RGB_DITHER_MAX,
                            img->buf + x + y*img->rowstride,
                            img->rowstride, img->cmap);
                    else if ((depth == DISPLAY_DEPTH_16) && img->rgbbuf)
                        gdk_draw_rgb_image(widget->window,
                            widget->style->fg_gc[GTK_STATE_NORMAL],
                            x, y, width, height,
                            GDK_RGB_DITHER_MAX,
                            img->rgbbuf + x*3 + y*img->width*3,
                            img->width * 3);
                    break;
                case DISPLAY_COLORS_GRAY:
                    if (depth == DISPLAY_DEPTH_8)
                        gdk_draw_gray_image(widget->window,
                            widget->style->fg_gc[GTK_STATE_NORMAL],
                            x, y, width, height,
                            GDK_RGB_DITHER_MAX,
                            img->buf + x + y*img->rowstride,
                            img->rowstride);
                    break;
                case DISPLAY_COLORS_RGB:
                    if (depth == DISPLAY_DEPTH_8) {
                        if (img->rgbbuf)
                            gdk_draw_rgb_image(widget->window,
                                widget->style->fg_gc[GTK_STATE_NORMAL],
                                x, y, width, height,
                                GDK_RGB_DITHER_MAX,
                                img->rgbbuf + x*3 + y*img->width*3,
                                img->width * 3);
                        else
                            gdk_draw_rgb_image(widget->window,
                                widget->style->fg_gc[GTK_STATE_NORMAL],
                                x, y, width, height,
                                GDK_RGB_DITHER_MAX,
                                img->buf + x*3 + y*img->rowstride,
                                img->rowstride);
                    }
                    break;
                case DISPLAY_COLORS_CMYK:
                    if (((depth == DISPLAY_DEPTH_1) ||
                        (depth == DISPLAY_DEPTH_8)) && img->rgbbuf)
                        gdk_draw_rgb_image(widget->window,
                            widget->style->fg_gc[GTK_STATE_NORMAL],
                            x, y, width, height,
                            GDK_RGB_DITHER_MAX,
                            img->rgbbuf + x*3 + y*img->width*3,
                            img->width * 3);
                    break;
                case DISPLAY_COLORS_SEPARATION:
                    if ((depth == DISPLAY_DEPTH_8) && img->rgbbuf)
                        gdk_draw_rgb_image(widget->window,
                            widget->style->fg_gc[GTK_STATE_NORMAL],
                            x, y, width, height,
                            GDK_RGB_DITHER_MAX,
                            img->rgbbuf + x*3 + y*img->width*3,
                            img->width * 3);
                    break;
            }
        }
    }
    return TRUE;
}