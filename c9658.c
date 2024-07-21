_fill_a8_lerp_spans (void *abstract_renderer, int y, int h,
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
		uint8_t *d = r->u.fill.data + r->u.fill.stride*y + spans[0].x;
		uint16_t p = (uint16_t)a * r->u.fill.pixel + 0x7f;
		uint16_t ia = ~a;
		while (len--) {
		    uint16_t t = *d*ia + p;
		    *d++ = (t + (t>>8)) >> 8;
		}
	    }
	    spans++;
	} while (--num_spans > 1);
    } else {
	do {
	    uint8_t a = mul8_8 (spans[0].coverage, r->op);
	    if (a) {
		int yy = y, hh = h;
		uint16_t p = (uint16_t)a * r->u.fill.pixel + 0x7f;
		uint16_t ia = ~a;
		do {
		    int len = spans[1].x - spans[0].x;
		    uint8_t *d = r->u.fill.data + r->u.fill.stride*yy + spans[0].x;
		    while (len--) {
			uint16_t t = *d*ia + p;
			*d++ = (t + (t>>8)) >> 8;
		    }
		    yy++;
		} while (--hh);
	    }
	    spans++;
	} while (--num_spans > 1);
    }

    return CAIRO_STATUS_SUCCESS;
}