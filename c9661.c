_fill_xrgb32_lerp_spans (void *abstract_renderer, int y, int h,
			 const cairo_half_open_span_t *spans, unsigned num_spans)
{
    cairo_image_span_renderer_t *r = abstract_renderer;

    if (num_spans == 0)
	return CAIRO_STATUS_SUCCESS;

    if (likely(h == 1)) {
	do {
	    uint8_t a = mul8_8 (spans[0].coverage, r->op);
	    if (a) {
		int len = spans[1].x - spans[0].x;
		uint32_t *d = (uint32_t*)(r->u.fill.data + r->u.fill.stride*y + spans[0].x*4);
		while (len--) {
		    *d = lerp8x4 (r->u.fill.pixel, a, *d);
		    d++;
		}
	    }
	    spans++;
	} while (--num_spans > 1);
    } else {
	do {
	    uint8_t a = mul8_8 (spans[0].coverage, r->op);
	    if (a) {
		int yy = y, hh = h;
		do {
		    int len = spans[1].x - spans[0].x;
		    uint32_t *d = (uint32_t *)(r->u.fill.data + r->u.fill.stride*yy + spans[0].x*4);
		    while (len--) {
			*d = lerp8x4 (r->u.fill.pixel, a, *d);
			d++;
		    }
		    yy++;
		} while (--hh);
	    }
	    spans++;
	} while (--num_spans > 1);
    }

    return CAIRO_STATUS_SUCCESS;
}