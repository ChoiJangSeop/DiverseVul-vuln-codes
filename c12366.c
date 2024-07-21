gerbv_gdk_draw_prim21(GdkPixmap *pixmap, GdkGC *gc, double *p, 
		  double scale, gint x, gint y)
{
    const int exposure_idx = 0;
    const int width_idx = 1;
    const int height_idx = 2;
    const int exp_x_idx = 3;
    const int exp_y_idx = 4;
    const int rotation_idx = 5;
    const int nuf_points = 4;
    GdkPoint points[nuf_points];
    GdkColor color;
    GdkGC *local_gc = gdk_gc_new(pixmap);
    int half_width, half_height;
    int i;

    half_width = (int)round(p[width_idx] * scale / 2.0);
    half_height =(int)round(p[height_idx] * scale / 2.0);

    points[0].x = half_width;
    points[0].y = half_height;

    points[1].x = half_width;
    points[1].y = -half_height;

    points[2].x = -half_width;
    points[2].y = -half_height;

    points[3].x = -half_width;
    points[3].y = half_height;

    for (i = 0; i < nuf_points; i++) {
	points[i].x += (int)(p[exp_x_idx] * scale);
	points[i].y -= (int)(p[exp_y_idx] * scale);
	points[i] = rotate_point(points[i], p[rotation_idx]);
	points[i].x += x;
	points[i].y += y;
    }

    gdk_gc_copy(local_gc, gc);

    /* Exposure */
    if (p[exposure_idx] == 0.0) {
	color.pixel = 0;
	gdk_gc_set_foreground(local_gc, &color);
    }

    gdk_draw_polygon(pixmap, local_gc, 1, points, nuf_points);

    gdk_gc_unref(local_gc);

    return;
} /* gerbv_gdk_draw_prim21 */