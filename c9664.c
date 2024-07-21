_inplace_spans (void *abstract_renderer,
		int y, int h,
		const cairo_half_open_span_t *spans,
		unsigned num_spans)
{
    cairo_image_span_renderer_t *r = abstract_renderer;
    uint8_t *mask;
    int x0, x1;

    if (num_spans == 0)
	return CAIRO_STATUS_SUCCESS;

    if (num_spans == 2 && spans[0].coverage == 0xff) {
	pixman_image_composite32 (r->op, r->src, NULL, r->u.composite.dst,
				  spans[0].x + r->u.composite.src_x,
				  y + r->u.composite.src_y,
				  0, 0,
				  spans[0].x, y,
				  spans[1].x - spans[0].x, h);
	return CAIRO_STATUS_SUCCESS;
    }

    mask = (uint8_t *)pixman_image_get_data (r->mask);
    x1 = x0 = spans[0].x;
    do {
	int len = spans[1].x - spans[0].x;
	*mask++ = spans[0].coverage;
	if (len > 1) {
	    if (len >= r->u.composite.run_length && spans[0].coverage == 0xff) {
		if (x1 != x0) {
		    pixman_image_composite32 (r->op, r->src, r->mask, r->u.composite.dst,
					      x0 + r->u.composite.src_x,
					      y + r->u.composite.src_y,
					      0, 0,
					      x0, y,
					      x1 - x0, h);
		}
		pixman_image_composite32 (r->op, r->src, NULL, r->u.composite.dst,
					  spans[0].x + r->u.composite.src_x,
					  y + r->u.composite.src_y,
					  0, 0,
					  spans[0].x, y,
					  len, h);
		mask = (uint8_t *)pixman_image_get_data (r->mask);
		x0 = spans[1].x;
	    } else if (spans[0].coverage == 0x0) {
		if (x1 != x0) {
		    pixman_image_composite32 (r->op, r->src, r->mask, r->u.composite.dst,
					      x0 + r->u.composite.src_x,
					      y + r->u.composite.src_y,
					      0, 0,
					      x0, y,
					      x1 - x0, h);
		}
		mask = (uint8_t *)pixman_image_get_data (r->mask);
		x0 = spans[1].x;
	    }else {
		memset (mask, spans[0].coverage, --len);
		mask += len;
	    }
	}
	x1 = spans[1].x;
	spans++;
    } while (--num_spans > 1);

    if (x1 != x0) {
	pixman_image_composite32 (r->op, r->src, r->mask, r->u.composite.dst,
				  x0 + r->u.composite.src_x,
				  y + r->u.composite.src_y,
				  0, 0,
				  x0, y,
				  x1 - x0, h);
    }

    return CAIRO_STATUS_SUCCESS;
}