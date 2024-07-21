static int display_size(void *handle, void *device, int width, int height,
        int raster, unsigned int format, unsigned char *pimage)
{
    IMAGE *img = image_find(handle, device);
    int color;
    int depth;
    int i;
    if (img == NULL)
        return -1;

    if (img->cmap)
        gdk_rgb_cmap_free(img->cmap);
    img->cmap = NULL;
    if (img->rgbbuf)
        free(img->rgbbuf);
    img->rgbbuf = NULL;

    img->width = width;
    img->height = height;
    img->rowstride = raster;
    img->buf = pimage;
    img->format = format;

    /* Reset separations */
    for (i=0; i<IMAGE_DEVICEN_MAX; i++) {
        img->devicen[i].used = 0;
        img->devicen[i].visible = 1;
        memset(img->devicen[i].name, 0, sizeof(img->devicen[i].name));
        img->devicen[i].cyan = 0;
        img->devicen[i].magenta = 0;
        img->devicen[i].yellow = 0;
        img->devicen[i].black = 0;
    }

    color = img->format & DISPLAY_COLORS_MASK;
    depth = img->format & DISPLAY_DEPTH_MASK;
    switch (color) {
        case DISPLAY_COLORS_NATIVE:
            if (depth == DISPLAY_DEPTH_8) {
                /* palette of 96 colors */
                guint32 color[96];
                int i;
                int one = 255 / 3;
                for (i=0; i<96; i++) {
                    /* 0->63 = 00RRGGBB, 64->95 = 010YYYYY */
                    if (i < 64) {
                        color[i] =
                            (((i & 0x30) >> 4) * one << 16) + 	/* r */
                            (((i & 0x0c) >> 2) * one << 8) + 	/* g */
                            (i & 0x03) * one;		        /* b */
                    }
                    else {
                        int val = i & 0x1f;
                        val = (val << 3) + (val >> 2);
                        color[i] = (val << 16) + (val << 8) + val;
                    }
                }
                img->cmap = gdk_rgb_cmap_new(color, 96);
                break;
            }
            else if (depth == DISPLAY_DEPTH_16) {
                /* need to convert to 24RGB */
                img->rgbbuf = (guchar *)malloc(width * height * 3);
                if (img->rgbbuf == NULL)
                    return -1;
            }
            else
                return e_rangecheck;	/* not supported */
        case DISPLAY_COLORS_GRAY:
            if (depth == DISPLAY_DEPTH_8)
                break;
            else
                return -1;	/* not supported */
        case DISPLAY_COLORS_RGB:
            if (depth == DISPLAY_DEPTH_8) {
                if (((img->format & DISPLAY_ALPHA_MASK) == DISPLAY_ALPHA_NONE)
                    && ((img->format & DISPLAY_ENDIAN_MASK)
                        == DISPLAY_BIGENDIAN))
                    break;
                else {
                    /* need to convert to 24RGB */
                    img->rgbbuf = (guchar *)malloc(width * height * 3);
                    if (img->rgbbuf == NULL)
                        return -1;
                }
            }
            else
                return -1;	/* not supported */
            break;
        case DISPLAY_COLORS_CMYK:
            if ((depth == DISPLAY_DEPTH_1) || (depth == DISPLAY_DEPTH_8)) {
                /* need to convert to 24RGB */
                img->rgbbuf = (guchar *)malloc(width * height * 3);
                if (img->rgbbuf == NULL)
                    return -1;
                /* We already know about the CMYK components */
                img->devicen[0].used = 1;
                img->devicen[0].cyan = 65535;
                strncpy(img->devicen[0].name, "Cyan",
                    sizeof(img->devicen[0].name));
                img->devicen[1].used = 1;
                img->devicen[1].magenta = 65535;
                strncpy(img->devicen[1].name, "Magenta",
                    sizeof(img->devicen[1].name));
                img->devicen[2].used = 1;
                img->devicen[2].yellow = 65535;
                strncpy(img->devicen[2].name, "Yellow",
                    sizeof(img->devicen[2].name));
                img->devicen[3].used = 1;
                img->devicen[3].black = 65535;
                strncpy(img->devicen[3].name, "Black",
                    sizeof(img->devicen[3].name));
            }
            else
                return -1;	/* not supported */
            break;
        case DISPLAY_COLORS_SEPARATION:
            /* we can't display this natively */
            /* we will convert it just before displaying */
            if (depth != DISPLAY_DEPTH_8)
                return -1;	/* not supported */
            img->rgbbuf = (guchar *)malloc(width * height * 3);
            if (img->rgbbuf == NULL)
                return -1;
            break;
    }

    if ((color == DISPLAY_COLORS_CMYK) ||
        (color == DISPLAY_COLORS_SEPARATION)) {
        if (!img->cmyk_bar) {
            /* add bar to select separation */
            img->cmyk_bar = gtk_hbox_new(FALSE, 0);
            gtk_box_pack_start(GTK_BOX(img->vbox), img->cmyk_bar,
                FALSE, FALSE, 0);
            for (i=0; i<IMAGE_DEVICEN_MAX; i++) {
               img->separation[i] =
                window_add_button(img, img->devicen[i].name,
                   signal_separation[i]);
            }
            img->show_as_gray = gtk_check_button_new_with_label("Show as Gray");
            gtk_box_pack_end(GTK_BOX(img->cmyk_bar), img->show_as_gray,
                FALSE, FALSE, 5);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(img->show_as_gray),
                FALSE);
            gtk_signal_connect(GTK_OBJECT(img->show_as_gray), "clicked",
                signal_show_as_gray, img);
            gtk_widget_show(img->show_as_gray);
        }
        gtk_widget_show(img->cmyk_bar);
    }
    else {
        if (img->cmyk_bar)
            gtk_widget_hide(img->cmyk_bar);
    }

    window_resize(img);
    if (!(GTK_WIDGET_FLAGS(img->window) & GTK_VISIBLE))
        gtk_widget_show(img->window);

    gtk_main_iteration_do(FALSE);
    return 0;
}