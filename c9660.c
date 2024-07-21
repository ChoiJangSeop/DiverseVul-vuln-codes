_blit_xrgb32_lerp_spans (void *abstract_renderer, int y, int h,
			 const cairo_half_open_span_t *spans, unsigned num_spans)
{
    cairo_image_span_renderer_t *r = abstract_renderer;

    if (num_spans == 0)
	return CAIRO_STATUS_SUCCESS;

    if (likely(h == 1)) {
	uint8_t *src = r->u.blit.src_data + y*r->u.blit.src_stride;
	uint8_t *dst = r->u.blit.data + y*r->u.blit.stride;
	do {
	    uint8_t a = mul8_8 (spans[0].coverage, r->op);
	    if (a) {
		uint32_t *s = (uint32_t*)src + spans[0].x;
		uint32_t *d = (uint32_t*)dst + spans[0].x;
		int len = spans[1].x - spans[0].x;
		if (a == 0xff) {
		    if (len == 1)
			*d = *s;
		    else
			memcpy(d, s, len*4);
		} else {
		    while (len--) {
			*d = lerp8x4 (*s, a, *d);
			s++, d++;
		    }
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
		    uint32_t *s = (uint32_t *)(r->u.blit.src_data + yy*r->u.blit.src_stride + spans[0].x * 4);
		    uint32_t *d = (uint32_t *)(r->u.blit.data + yy*r->u.blit.stride + spans[0].x * 4);
		    int len = spans[1].x - spans[0].x;
		    if (a == 0xff) {
			if (len == 1)
			    *d = *s;
			else
			    memcpy(d, s, len * 4);
		    } else {
			while (len--) {
			    *d = lerp8x4 (*s, a, *d);
			    s++, d++;
			}
		    }
		    yy++;
		} while (--hh);
	    }
	    spans++;
	} while (--num_spans > 1);
    }

    return CAIRO_STATUS_SUCCESS;
}