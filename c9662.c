inplace_renderer_init (cairo_image_span_renderer_t	*r,
		       const cairo_composite_rectangles_t *composite,
		       cairo_antialias_t		 antialias,
		       cairo_bool_t			 needs_clip)
{
    cairo_image_surface_t *dst = (cairo_image_surface_t *)composite->surface;
    uint8_t *buf;

    if (composite->mask_pattern.base.type != CAIRO_PATTERN_TYPE_SOLID)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    r->base.render_rows = NULL;
    r->op = composite->mask_pattern.solid.color.alpha_short >> 8;

    if (composite->source_pattern.base.type == CAIRO_PATTERN_TYPE_SOLID) {
	const cairo_color_t *color;

	color = &composite->source_pattern.solid.color;
	if (composite->op == CAIRO_OPERATOR_CLEAR)
	    color = CAIRO_COLOR_TRANSPARENT;

	if (fill_reduces_to_source (composite->op, color, dst) &&
	    color_to_pixel (color, dst->pixman_format, &r->u.fill.pixel)) {
	    /* Use plain C for the fill operations as the span length is
	     * typically small, too small to payback the startup overheads of
	     * using SSE2 etc.
	     */
	    if (r->op == 0xff) {
		switch (dst->format) {
		case CAIRO_FORMAT_A8:
		    r->base.render_rows = _fill_a8_lerp_opaque_spans;
		    break;
		case CAIRO_FORMAT_RGB24:
		case CAIRO_FORMAT_ARGB32:
		    r->base.render_rows = _fill_xrgb32_lerp_opaque_spans;
		    break;
		case CAIRO_FORMAT_A1:
		case CAIRO_FORMAT_RGB16_565:
		case CAIRO_FORMAT_RGB30:
		case CAIRO_FORMAT_INVALID:
		default: break;
		}
	    } else {
		switch (dst->format) {
		case CAIRO_FORMAT_A8:
		    r->base.render_rows = _fill_a8_lerp_spans;
		    break;
		case CAIRO_FORMAT_RGB24:
		case CAIRO_FORMAT_ARGB32:
		    r->base.render_rows = _fill_xrgb32_lerp_spans;
		    break;
		case CAIRO_FORMAT_A1:
		case CAIRO_FORMAT_RGB16_565:
		case CAIRO_FORMAT_RGB30:
		case CAIRO_FORMAT_INVALID:
		default: break;
		}
	    }
	    r->u.fill.data = dst->data;
	    r->u.fill.stride = dst->stride;
	}
    } else if ((dst->format == CAIRO_FORMAT_ARGB32 || dst->format == CAIRO_FORMAT_RGB24) &&
	       (composite->op == CAIRO_OPERATOR_SOURCE ||
		(composite->op == CAIRO_OPERATOR_OVER &&
		 (dst->base.is_clear || (dst->base.content & CAIRO_CONTENT_ALPHA) == 0))) &&
	       composite->source_pattern.base.type == CAIRO_PATTERN_TYPE_SURFACE &&
	       composite->source_pattern.surface.surface->backend->type == CAIRO_SURFACE_TYPE_IMAGE &&
	       to_image_surface(composite->source_pattern.surface.surface)->format == dst->format)
    {
       cairo_image_surface_t *src =
	   to_image_surface(composite->source_pattern.surface.surface);
       int tx, ty;

	if (_cairo_matrix_is_integer_translation(&composite->source_pattern.base.matrix,
						 &tx, &ty) &&
	    composite->bounded.x + tx >= 0 &&
	    composite->bounded.y + ty >= 0 &&
	    composite->bounded.x + composite->bounded.width + tx <= src->width &&
	    composite->bounded.y + composite->bounded.height + ty <= src->height) {

	    assert(PIXMAN_FORMAT_BPP(dst->pixman_format) == 32);
	    r->u.blit.stride = dst->stride;
	    r->u.blit.data = dst->data;
	    r->u.blit.src_stride = src->stride;
	    r->u.blit.src_data = src->data + src->stride * ty + tx * 4;
	    r->base.render_rows = _blit_xrgb32_lerp_spans;
	}
    }
    if (r->base.render_rows == NULL) {
	unsigned int width;
	const cairo_pattern_t *src = &composite->source_pattern.base;

	if (r->op != 0xff)
	    return CAIRO_INT_STATUS_UNSUPPORTED;

	if (composite->is_bounded == 0)
	    return CAIRO_INT_STATUS_UNSUPPORTED;

	width = (composite->bounded.width + 3) & ~3;
	r->base.render_rows = _inplace_spans;
	r->u.composite.run_length = 8;
	if (src->type == CAIRO_PATTERN_TYPE_LINEAR ||
	    src->type == CAIRO_PATTERN_TYPE_RADIAL)
		r->u.composite.run_length = 256;
	if (dst->base.is_clear &&
	    (composite->op == CAIRO_OPERATOR_SOURCE ||
	     composite->op == CAIRO_OPERATOR_OVER ||
	     composite->op == CAIRO_OPERATOR_ADD)) {
	    r->op = PIXMAN_OP_SRC;
	} else if (composite->op == CAIRO_OPERATOR_SOURCE) {
	    r->base.render_rows = _inplace_src_spans;
	    r->u.composite.mask_y = r->composite->unbounded.y;
	    width = (composite->unbounded.width + 3) & ~3;
	} else if (composite->op == CAIRO_OPERATOR_CLEAR) {
	    r->op = PIXMAN_OP_OUT_REVERSE;
	    src = NULL;
	} else {
	    r->op = _pixman_operator (composite->op);
	}

	r->src = _pixman_image_for_pattern (dst, src, FALSE,
					    &composite->bounded,
					    &composite->source_sample_area,
					    &r->u.composite.src_x, &r->u.composite.src_y);
	if (unlikely (r->src == NULL))
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	/* Create an effectively unbounded mask by repeating the single line */
	buf = r->buf;
	if (width > sizeof (r->buf)) {
	    buf = malloc (width);
	    if (unlikely (buf == NULL)) {
		pixman_image_unref (r->src);
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);
	    }
	}
	r->mask = pixman_image_create_bits (PIXMAN_a8,
					    width, composite->unbounded.height,
					    (uint32_t *)buf, 0);
	if (unlikely (r->mask == NULL)) {
	    pixman_image_unref (r->src);
	    if (buf != r->buf)
		free (buf);
	    return _cairo_error(CAIRO_STATUS_NO_MEMORY);
	}

	if (buf != r->buf)
	    pixman_image_set_destroy_function (r->mask, free_pixels, buf);

	r->u.composite.dst = dst->pixman_image;
    }

    r->bpp = PIXMAN_FORMAT_BPP(dst->pixman_format);

    return CAIRO_INT_STATUS_SUCCESS;
}