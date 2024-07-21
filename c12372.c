gerbv_gdk_draw_prim4(GdkPixmap *pixmap, GdkGC *gc, double *p, 
		     double scale, gint x, gint y)
{
    const int exposure_idx = 0;
    const int nuf_points_idx = 1;
    const int first_x_idx = 2;
    const int first_y_idx = 3;
    const int rotext_idx = 4;
    GdkGC *local_gc = gdk_gc_new(pixmap);
    int nuf_points, point;
    double rotation;
    GdkPoint *points;
    GdkColor color;

    /* Include start point */
    nuf_points = (int)p[nuf_points_idx] + 1;
    points = g_new(GdkPoint, nuf_points);
    if (!points) {
	g_free(points);
	return;
    }

    rotation = p[(nuf_points - 1) * 2 + rotext_idx];
    for (point = 0; point < nuf_points; point++) {
	points[point].x = (int)round(scale * p[point * 2 + first_x_idx]);
	points[point].y = -(int)round(scale * p[point * 2 + first_y_idx]);
	if (rotation != 0.0)
	    points[point] = rotate_point(points[point], rotation);
	points[point].x += x;
	points[point].y += y;
    }

    gdk_gc_copy(local_gc, gc);

    /* Exposure */
    if (p[exposure_idx] == 0.0) {
	color.pixel = 0;
	gdk_gc_set_foreground(local_gc, &color);
    }

    gdk_gc_set_line_attributes(local_gc, 
			       1, /* outline always 1 pixels */
			       GDK_LINE_SOLID, 
			       GDK_CAP_BUTT, 
			       GDK_JOIN_MITER);
    gdk_draw_polygon(pixmap, local_gc, 1, points, nuf_points);

    g_free(points);

    gdk_gc_unref(local_gc);

    return;
} /* gerbv_gdk_draw_prim4 */