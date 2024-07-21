gerbv_gdk_draw_prim22(GdkPixmap *pixmap, GdkGC *gc, double *p, 
		  double scale, gint x, gint y)
{
    const int exposure_idx = 0;
    const int width_idx = 1;
    const int height_idx = 2;
    const int x_lower_left_idx = 3;
    const int y_lower_left_idx = 4;
    const int rotation_idx = 5;
    const int nuf_points = 4;
    GdkPoint points[nuf_points];
    GdkGC *local_gc = gdk_gc_new(pixmap);
    GdkColor color;
    int i;

    points[0].x = (int)round(p[x_lower_left_idx] * scale);
    points[0].y = (int)round(p[y_lower_left_idx] * scale);

    points[1].x = (int)round((p[x_lower_left_idx] + p[width_idx])
			     * scale);
    points[1].y = (int)round(p[y_lower_left_idx] * scale);

    points[2].x = (int)round((p[x_lower_left_idx]  + p[width_idx])
			     * scale);
    points[2].y = (int)round((p[y_lower_left_idx]  + p[height_idx])
			     * scale);

    points[3].x = (int)round(p[x_lower_left_idx] * scale);
    points[3].y = (int)round((p[y_lower_left_idx] + p[height_idx])
			     * scale);

    for (i = 0; i < nuf_points; i++) {
	points[i] = rotate_point(points[i], -p[rotation_idx]);
	points[i].x = x + points[i].x;
	points[i].y = y - points[i].y;
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
} /* gerbv_gdk_draw_prim22 */