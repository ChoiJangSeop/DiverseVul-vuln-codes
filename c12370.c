gerbv_gdk_draw_prim7(GdkPixmap *pixmap, GdkGC *gc, double *p, 
		 double scale, gint x, gint y)
{
    const int outside_dia_idx = 2;
    const int inside_dia_idx = 3;
    const int ch_thickness_idx = 4;
    const int rotation_idx = 5;
    const gint full_circle = 23360;
    GdkGCValues gc_val;
    int diameter, i;
    GdkGC *local_gc = gdk_gc_new(pixmap);
    GdkPoint point[4];
    double ci_thickness = (p[outside_dia_idx] - 
			   p[inside_dia_idx]) / 2.0;

    gdk_gc_copy(local_gc, gc);
    gdk_gc_set_line_attributes(local_gc, 
			       (int)round(scale * ci_thickness),
			       GDK_LINE_SOLID, 
			       GDK_CAP_BUTT, 
			       GDK_JOIN_MITER);

    /* 
     * Non filled circle 
     */
    diameter = (p[inside_dia_idx] + ci_thickness) * scale;
    gdk_draw_arc(pixmap, local_gc, 0, x - diameter / 2, y - diameter / 2, 
		 diameter, diameter, 0, full_circle);

    /*
     * Cross hair
     */ 
    /* Calculate the end points of the crosshair */    
    /* GDK doesn't always remove all of the circle (round of error probably)
       I extend the crosshair line with 2 (one pixel in each end) to make 
       sure all of the circle is removed with the crosshair */
    for (i = 0; i < 4; i++) {
	point[i].x = round((p[outside_dia_idx] / 2.0) * scale) + 2;
	point[i].y = 0;
	point[i] = rotate_point(point[i], p[rotation_idx] + 90 * i);
	point[i].x += x;
	point[i].y += y;
    }

    gdk_gc_set_line_attributes(local_gc, 
			       (int)round(scale * p[ch_thickness_idx]),
			       GDK_LINE_SOLID, 
			       GDK_CAP_BUTT, 
			       GDK_JOIN_MITER);

    /* The cross hair should "cut out" parts of the circle, hence inverse */
    gdk_gc_get_values(local_gc, &gc_val);
    if (gc_val.foreground.pixel == 1)
	gc_val.foreground.pixel = 0;
    else
	gc_val.foreground.pixel = 1;
    gdk_gc_set_foreground(local_gc, &(gc_val.foreground));

    /* Draw the actual cross */
    gdk_draw_line(pixmap, local_gc, 
		  point[0].x, point[0].y, point[2].x, point[2].y);
    gdk_draw_line(pixmap, local_gc,
		  point[1].x, point[1].y, point[3].x, point[3].y);

    gdk_gc_unref(local_gc);

    return;
} /* gerbv_gdk_draw_prim7 */