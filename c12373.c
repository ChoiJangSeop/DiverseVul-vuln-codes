gerbv_gdk_draw_prim20(GdkPixmap *pixmap, GdkGC *gc, double *p, 
		  double scale, gint x, gint y)
{
    const int exposure_idx = 0;
    const int linewidth_idx = 1;
    const int start_x_idx = 2;
    const int start_y_idx = 3;
    const int end_x_idx = 4;
    const int end_y_idx = 5;
    const int rotation_idx = 6;
    const int nuf_points = 2;
    GdkGC *local_gc = gdk_gc_new(pixmap);
    GdkPoint points[nuf_points];
    GdkColor color;
    int i;

    gdk_gc_copy(local_gc, gc);

    /* Exposure */
    if (p[exposure_idx] == 0.0) {
	color.pixel = 0;
	gdk_gc_set_foreground(local_gc, &color);
    }

    gdk_gc_set_line_attributes(local_gc, 
			       (int)round(scale * p[linewidth_idx]),
			       GDK_LINE_SOLID, 
			       GDK_CAP_BUTT, 
			       GDK_JOIN_MITER);

    points[0].x = (p[start_x_idx] * scale);
    points[0].y = (p[start_y_idx] * scale);
    points[1].x = (p[end_x_idx] * scale);
    points[1].y = (p[end_y_idx] * scale);

    for (i = 0; i < nuf_points; i++) {
	points[i] = rotate_point(points[i], -p[rotation_idx]);
	points[i].x = x + points[i].x;
	points[i].y = y - points[i].y;
    }

    gdk_draw_line(pixmap, local_gc, 
		  points[0].x, points[0].y, 
		  points[1].x, points[1].y);
    
    gdk_gc_unref(local_gc);

    return;
} /* gerbv_gdk_draw_prim20 */