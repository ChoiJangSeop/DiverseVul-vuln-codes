gerbv_gdk_draw_prim6(GdkPixmap *pixmap, GdkGC *gc, double *p, 
		 double scale, gint x, gint y)
{
    const int outside_dia_idx = 2;
    const int ci_thickness_idx = 3;
    const int gap_idx = 4;
    const int nuf_circles_idx = 5;
    const int ch_thickness_idx = 6;
    const int ch_length_idx = 7;
    const int rotation_idx = 8;
    GdkGC *local_gc = gdk_gc_new(pixmap);
    double real_dia;
    double real_dia_diff;
    int circle;
    GdkPoint crosshair[4];
    int point;

    gdk_gc_copy(local_gc, gc);
    gdk_gc_set_line_attributes(local_gc, 
			       (int)round(scale * p[ci_thickness_idx]),
			       GDK_LINE_SOLID, 
			       GDK_CAP_BUTT, 
			       GDK_JOIN_MITER);

    real_dia = p[outside_dia_idx] -  p[ci_thickness_idx] / 2.0;
    real_dia_diff = 2*(p[gap_idx] + p[ci_thickness_idx]);

    for (circle = 0; circle != (int)p[nuf_circles_idx];  circle++) {
	/* 
	 * Non filled circle 
	 */
	const gint full_circle = 23360;
	gint dia = (real_dia - real_dia_diff * circle) * scale;
	if (dia >= 0){
		gdk_draw_arc(pixmap, local_gc, 0, x - dia / 2, y - dia / 2, 
				dia, dia, 0, full_circle);
	}
    }

    /*
     * Cross Hair 
     */
    memset(crosshair, 0, sizeof(GdkPoint) * 4);
    crosshair[0].x = (int)((p[ch_length_idx] / 2.0) * scale);
    /*crosshair[0].y = 0;*/
    crosshair[1].x = -crosshair[0].x;
    /*crosshair[1].y = 0;*/
    /*crosshair[2].x = 0;*/
    crosshair[2].y = crosshair[0].x;
    /*crosshair[3].x = 0;*/
    crosshair[3].y = -crosshair[0].x;

    gdk_gc_set_line_attributes(local_gc, 
			       (int)round(scale * p[ch_thickness_idx]),
			       GDK_LINE_SOLID, 
			       GDK_CAP_BUTT, 
			       GDK_JOIN_MITER);

    for (point = 0; point < 4; point++) {
	crosshair[point] = rotate_point(crosshair[point], 
					p[rotation_idx]);
	crosshair[point].x += x;
	crosshair[point].y += y;
    }
    gdk_draw_line(pixmap, local_gc, 
		  crosshair[0].x, crosshair[0].y, 
		  crosshair[1].x, crosshair[1].y);
    gdk_draw_line(pixmap, local_gc, 
		  crosshair[2].x, crosshair[2].y, 
		  crosshair[3].x, crosshair[3].y);

    gdk_gc_unref(local_gc);

    return;
} /* gerbv_gdk_draw_prim6 */