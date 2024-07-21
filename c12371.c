gerbv_gdk_draw_prim5(GdkPixmap *pixmap, GdkGC *gc, double *p, 
		     double scale, gint x, gint y)
{
    const int exposure_idx = 0;
    const int nuf_vertices_idx = 1;
    const int center_x_idx = 2;
    const int center_y_idx = 3;
    const int diameter_idx = 4;
    const int rotation_idx = 5;
    int nuf_vertices, i;
    double vertex, tick, rotation, radius;
    GdkPoint *points;
    GdkGC *local_gc = gdk_gc_new(pixmap);
    GdkColor color;

    nuf_vertices = (int)p[nuf_vertices_idx];
    points = g_new(GdkPoint, nuf_vertices);
    if (!points) {
	g_free(points);
	return;
    }

    gdk_gc_copy(local_gc, gc);

    /* Exposure */
    if (p[exposure_idx] == 0.0) {
	color.pixel = 0;
	gdk_gc_set_foreground(local_gc, &color);
    }

    tick = 2 * M_PI / (double)nuf_vertices;
    rotation = DEG2RAD(-p[rotation_idx]);
    radius = p[diameter_idx] / 2.0;
    for (i = 0; i < nuf_vertices; i++) {
	vertex =  tick * (double)i + rotation;
	points[i].x = (int)round(scale * (radius * cos(vertex) + p[center_x_idx])) + x;
	points[i].y = (int)round(scale * (radius * sin(vertex) - p[center_y_idx])) + y;
    }

    gdk_draw_polygon(pixmap, local_gc, 1, points, nuf_vertices);

    gdk_gc_unref(local_gc);

    g_free(points);
    return;
} /* gerbv_gdk_draw_prim5 */