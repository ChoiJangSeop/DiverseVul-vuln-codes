gerbv_gdk_draw_prim1(GdkPixmap *pixmap, GdkGC *gc, double *p, 
		     double scale, gint x, gint y)
{
    const int exposure_idx = 0;
    const int diameter_idx = 1;
    const int x_offset_idx = 2;
    const int y_offset_idx = 3;
    const gint full_circle = 23360;
    GdkGC *local_gc = gdk_gc_new(pixmap);
    gint dia    = round(fabs(p[diameter_idx] * scale));
    gint real_x = x - dia / 2;
    gint real_y = y - dia / 2;
    GdkColor color;

    gdk_gc_copy(local_gc, gc);

    real_x += (int)(p[x_offset_idx] * (double)scale);
    real_y -= (int)(p[y_offset_idx] * (double)scale);

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

    /* 
     * A filled circle 
     */
    gdk_draw_arc(pixmap, local_gc, 1, real_x, real_y, dia, dia, 
		 0, full_circle);

    gdk_gc_unref(local_gc);

    return;
} /* gerbv_gdk_draw_prim1 */