static int display_sync(void *handle, void *device)
{
    IMAGE *img = image_find(handle, device);
    int color;
    int depth;
    int endian;
    int native555;
    int alpha;
    if (img == NULL)
        return -1;

    color = img->format & DISPLAY_COLORS_MASK;
    depth = img->format & DISPLAY_DEPTH_MASK;
    endian = img->format & DISPLAY_ENDIAN_MASK;
    native555 = img->format & DISPLAY_555_MASK;
    alpha = img->format & DISPLAY_ALPHA_MASK;

    if ((color == DISPLAY_COLORS_CMYK) ||
        (color == DISPLAY_COLORS_SEPARATION)) {
        /* check if separations have changed */
        int i;
        gchar *str;
        for (i=0; i<IMAGE_DEVICEN_MAX; i++) {
            gtk_label_get(
                GTK_LABEL(GTK_BIN(img->separation[i])->child), &str);
            if (!img->devicen[i].used)
                gtk_widget_hide(img->separation[i]);
            else if (strcmp(img->devicen[i].name, str) != 0) {
                /* text has changed, update it */
                gtk_label_set_text(
                    GTK_LABEL(GTK_BIN(img->separation[i])->child),
                    img->devicen[i].name);
                gtk_widget_show(img->separation[i]);
            }
        }
    }

    /* some formats need to be converted for use by GdkRgb */
    switch (color) {
        case DISPLAY_COLORS_NATIVE:
            if (depth == DISPLAY_DEPTH_16) {
              if (endian == DISPLAY_LITTLEENDIAN) {
                if (native555 == DISPLAY_NATIVE_555) {
                    /* BGR555 */
                    int x, y;
                    unsigned short w;
                    unsigned char value;
                    unsigned char *s, *d;
                    for (y = 0; y<img->height; y++) {
                        s = img->buf + y * img->rowstride;
                        d = img->rgbbuf + y * img->width * 3;
                        for (x=0; x<img->width; x++) {
                            w = s[0] + (s[1] << 8);
                            value = (w >> 10) & 0x1f;	/* red */
                            *d++ = (value << 3) + (value >> 2);
                            value = (w >> 5) & 0x1f;	/* green */
                            *d++ = (value << 3) + (value >> 2);
                            value = w & 0x1f;		/* blue */
                            *d++ = (value << 3) + (value >> 2);
                            s += 2;
                        }
                    }
                }
                else {
                    /* BGR565 */
                    int x, y;
                    unsigned short w;
                    unsigned char value;
                    unsigned char *s, *d;
                    for (y = 0; y<img->height; y++) {
                        s = img->buf + y * img->rowstride;
                        d = img->rgbbuf + y * img->width * 3;
                        for (x=0; x<img->width; x++) {
                            w = s[0] + (s[1] << 8);
                            value = (w >> 11) & 0x1f;	/* red */
                            *d++ = (value << 3) + (value >> 2);
                            value = (w >> 5) & 0x3f;	/* green */
                            *d++ = (value << 2) + (value >> 4);
                            value = w & 0x1f;		/* blue */
                            *d++ = (value << 3) + (value >> 2);
                            s += 2;
                        }
                    }
                }
              }
              else {
                if (native555 == DISPLAY_NATIVE_555) {
                    /* RGB555 */
                    int x, y;
                    unsigned short w;
                    unsigned char value;
                    unsigned char *s, *d;
                    for (y = 0; y<img->height; y++) {
                        s = img->buf + y * img->rowstride;
                        d = img->rgbbuf + y * img->width * 3;
                        for (x=0; x<img->width; x++) {
                            w = s[1] + (s[0] << 8);
                            value = (w >> 10) & 0x1f;	/* red */
                            *d++ = (value << 3) + (value >> 2);
                            value = (w >> 5) & 0x1f;	/* green */
                            *d++ = (value << 3) + (value >> 2);
                            value = w & 0x1f;		/* blue */
                            *d++ = (value << 3) + (value >> 2);
                            s += 2;
                        }
                    }
                }
                else {
                    /* RGB565 */
                    int x, y;
                    unsigned short w;
                    unsigned char value;
                    unsigned char *s, *d;
                    for (y = 0; y<img->height; y++) {
                        s = img->buf + y * img->rowstride;
                        d = img->rgbbuf + y * img->width * 3;
                        for (x=0; x<img->width; x++) {
                            w = s[1] + (s[0] << 8);
                            value = (w >> 11) & 0x1f;	/* red */
                            *d++ = (value << 3) + (value >> 2);
                            value = (w >> 5) & 0x3f;	/* green */
                            *d++ = (value << 2) + (value >> 4);
                            value = w & 0x1f;		/* blue */
                            *d++ = (value << 3) + (value >> 2);
                            s += 2;
                        }
                    }
                }
              }
            }
            break;
        case DISPLAY_COLORS_RGB:
            if ( (depth == DISPLAY_DEPTH_8) &&
                 ((alpha == DISPLAY_ALPHA_FIRST) ||
                  (alpha == DISPLAY_UNUSED_FIRST)) &&
                 (endian == DISPLAY_BIGENDIAN) ) {
                /* Mac format */
                int x, y;
                unsigned char *s, *d;
                for (y = 0; y<img->height; y++) {
                    s = img->buf + y * img->rowstride;
                    d = img->rgbbuf + y * img->width * 3;
                    for (x=0; x<img->width; x++) {
                        s++;		/* x = filler */
                        *d++ = *s++;	/* r */
                        *d++ = *s++;	/* g */
                        *d++ = *s++;	/* b */
                    }
                }
            }
            else if ( (depth == DISPLAY_DEPTH_8) &&
                      (endian == DISPLAY_LITTLEENDIAN) ) {
                if ((alpha == DISPLAY_UNUSED_LAST) ||
                    (alpha == DISPLAY_ALPHA_LAST)) {
                    /* Windows format + alpha = BGRx */
                    int x, y;
                    unsigned char *s, *d;
                    for (y = 0; y<img->height; y++) {
                        s = img->buf + y * img->rowstride;
                        d = img->rgbbuf + y * img->width * 3;
                        for (x=0; x<img->width; x++) {
                            *d++ = s[2];	/* r */
                            *d++ = s[1];	/* g */
                            *d++ = s[0];	/* b */
                            s += 4;
                        }
                    }
                }
                else if ((alpha == DISPLAY_UNUSED_FIRST) ||
                    (alpha == DISPLAY_ALPHA_FIRST)) {
                    /* xBGR */
                    int x, y;
                    unsigned char *s, *d;
                    for (y = 0; y<img->height; y++) {
                        s = img->buf + y * img->rowstride;
                        d = img->rgbbuf + y * img->width * 3;
                        for (x=0; x<img->width; x++) {
                            *d++ = s[3];	/* r */
                            *d++ = s[2];	/* g */
                            *d++ = s[1];	/* b */
                            s += 4;
                        }
                    }
                }
                else {
                    /* Windows BGR24 */
                    int x, y;
                    unsigned char *s, *d;
                    for (y = 0; y<img->height; y++) {
                        s = img->buf + y * img->rowstride;
                        d = img->rgbbuf + y * img->width * 3;
                        for (x=0; x<img->width; x++) {
                            *d++ = s[2];	/* r */
                            *d++ = s[1];	/* g */
                            *d++ = s[0];	/* b */
                            s += 3;
                        }
                    }
                }
            }
            break;
        case DISPLAY_COLORS_CMYK:
            if (depth == DISPLAY_DEPTH_8) {
                /* Separations */
                int x, y;
                int cyan, magenta, yellow, black;
                unsigned char *s, *d;
                int vc = img->devicen[0].visible;
                int vm = img->devicen[1].visible;
                int vy = img->devicen[2].visible;
                int vk = img->devicen[3].visible;
                int vall = vc && vm && vy && vk;
                int show_gray = (vc + vm + vy + vk == 1) && img->devicen_gray;
                for (y = 0; y<img->height; y++) {
                    s = img->buf + y * img->rowstride;
                    d = img->rgbbuf + y * img->width * 3;
                    for (x=0; x<img->width; x++) {
                        cyan = *s++;
                        magenta = *s++;
                        yellow = *s++;
                        black = *s++;
                        if (!vall) {
                            if (!vc)
                                cyan = 0;
                            if (!vm)
                                magenta = 0;
                            if (!vy)
                                yellow = 0;
                            if (!vk)
                                black = 0;
                            if (show_gray) {
                                black += cyan + magenta + yellow;
                                cyan = magenta = yellow = 0;
                            }
                        }
                        *d++ = (255-cyan)    * (255-black) / 255; /* r */
                        *d++ = (255-magenta) * (255-black) / 255; /* g */
                        *d++ = (255-yellow)  * (255-black) / 255; /* b */
                    }
                }
            }
            else if (depth == DISPLAY_DEPTH_1) {
                /* Separations */
                int x, y;
                int cyan, magenta, yellow, black;
                unsigned char *s, *d;
                int vc = img->devicen[0].visible;
                int vm = img->devicen[1].visible;
                int vy = img->devicen[2].visible;
                int vk = img->devicen[3].visible;
                int vall = vc && vm && vy && vk;
                int show_gray = (vc + vm + vy + vk == 1) && img->devicen_gray;
                int value;
                for (y = 0; y<img->height; y++) {
                    s = img->buf + y * img->rowstride;
                    d = img->rgbbuf + y * img->width * 3;
                    for (x=0; x<img->width; x++) {
                        value = s[x/2];
                        if (x & 0)
                            value >>= 4;
                        cyan = ((value >> 3) & 1) * 255;
                        magenta = ((value >> 2) & 1) * 255;
                        yellow = ((value >> 1) & 1) * 255;
                        black = (value & 1) * 255;
                        if (!vall) {
                            if (!vc)
                                cyan = 0;
                            if (!vm)
                                magenta = 0;
                            if (!vy)
                                yellow = 0;
                            if (!vk)
                                black = 0;
                            if (show_gray) {
                                black += cyan + magenta + yellow;
                                cyan = magenta = yellow = 0;
                            }
                        }
                        *d++ = (255-cyan)    * (255-black) / 255; /* r */
                        *d++ = (255-magenta) * (255-black) / 255; /* g */
                        *d++ = (255-yellow)  * (255-black) / 255; /* b */
                    }
                }
            }
            break;
        case DISPLAY_COLORS_SEPARATION:
            if (depth == DISPLAY_DEPTH_8) {
                int j;
                int x, y;
                unsigned char *s, *d;
                int cyan, magenta, yellow, black;
                int num_comp = 0;
                int value;
                int num_visible = 0;
                int show_gray = 0;
                IMAGE_DEVICEN *devicen = img->devicen;
                for (j=0; j<IMAGE_DEVICEN_MAX; j++) {
                    if (img->devicen[j].used) {
                       num_comp = j+1;
                       if (img->devicen[j].visible)
                            num_visible++;
                    }
                }
                if ((num_visible == 1) && img->devicen_gray)
                    show_gray = 1;

                for (y = 0; y<img->height; y++) {
                    s = img->buf + y * img->rowstride;
                    d = img->rgbbuf + y * img->width * 3;
                    for (x=0; x<img->width; x++) {
                        cyan = magenta = yellow = black = 0;
                        if (show_gray) {
                            for (j=0; j<num_comp; j++) {
                                devicen = &img->devicen[j];
                                if (devicen->visible && devicen->used)
                                    black += s[j];
                            }
                        }
                        else {
                            for (j=0; j<num_comp; j++) {
                                devicen = &img->devicen[j];
                                if (devicen->visible && devicen->used) {
                                    value = s[j];
                                    cyan    += value*devicen->cyan   /65535;
                                    magenta += value*devicen->magenta/65535;
                                    yellow  += value*devicen->yellow /65535;
                                    black   += value*devicen->black  /65535;
                                }
                            }
                        }
                        if (cyan > 255)
                           cyan = 255;
                        if (magenta > 255)
                           magenta = 255;
                        if (yellow > 255)
                           yellow = 255;
                        if (black > 255)
                           black = 255;
                        *d++ = (255-cyan)    * (255-black) / 255; /* r */
                        *d++ = (255-magenta) * (255-black) / 255; /* g */
                        *d++ = (255-yellow)  * (255-black) / 255; /* b */
                        s += 8;
                    }
                }
            }
            break;
    }

    if (img->window == NULL) {
        window_create(img);
        window_resize(img);
    }
    if (!(GTK_WIDGET_FLAGS(img->window) & GTK_VISIBLE))
        gtk_widget_show_all(img->window);

    gtk_widget_draw(img->darea, NULL);
    gtk_main_iteration_do(FALSE);
    return 0;
}