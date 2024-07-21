span_renderer_init (cairo_abstract_span_renderer_t	*_r,
		    const cairo_composite_rectangles_t *composite,
		    cairo_antialias_t			 antialias,
		    cairo_bool_t			 needs_clip)
{
    cairo_image_span_renderer_t *r = (cairo_image_span_renderer_t *)_r;
    cairo_image_surface_t *dst = (cairo_image_surface_t *)composite->surface;
    const cairo_pattern_t *source = &composite->source_pattern.base;
    cairo_operator_t op = composite->op;
    cairo_int_status_t status;

    TRACE ((stderr, "%s: antialias=%d, needs_clip=%d\n", __FUNCTION__,
	    antialias, needs_clip));

    if (needs_clip)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    r->composite = composite;
    r->mask = NULL;
    r->src = NULL;
    r->base.finish = NULL;

    status = mono_renderer_init (r, composite, antialias, needs_clip);
    if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	return status;

    status = inplace_renderer_init (r, composite, antialias, needs_clip);
    if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	return status;

    r->bpp = 0;

    if (op == CAIRO_OPERATOR_CLEAR) {
#if PIXMAN_HAS_OP_LERP
	op = PIXMAN_OP_LERP_CLEAR;
#else
	source = &_cairo_pattern_white.base;
	op = PIXMAN_OP_OUT_REVERSE;
#endif
    } else if (dst->base.is_clear &&
	       (op == CAIRO_OPERATOR_SOURCE ||
		op == CAIRO_OPERATOR_OVER ||
		op == CAIRO_OPERATOR_ADD)) {
	op = PIXMAN_OP_SRC;
    } else if (op == CAIRO_OPERATOR_SOURCE) {
#if PIXMAN_HAS_OP_LERP
	op = PIXMAN_OP_LERP_SRC;
#else
	return CAIRO_INT_STATUS_UNSUPPORTED;
#endif
    } else {
	op = _pixman_operator (op);
    }
    r->op = op;

    r->src = _pixman_image_for_pattern (dst, source, FALSE,
					&composite->unbounded,
					&composite->source_sample_area,
					&r->u.mask.src_x, &r->u.mask.src_y);
    if (unlikely (r->src == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    r->opacity = 1.0;
    if (composite->mask_pattern.base.type == CAIRO_PATTERN_TYPE_SOLID) {
	r->opacity = composite->mask_pattern.solid.color.alpha;
    } else {
	pixman_image_t *mask;
	int mask_x, mask_y;

	mask = _pixman_image_for_pattern (dst,
					  &composite->mask_pattern.base,
					  TRUE,
					  &composite->unbounded,
					  &composite->mask_sample_area,
					  &mask_x, &mask_y);
	if (unlikely (mask == NULL))
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	/* XXX Component-alpha? */
	if ((dst->base.content & CAIRO_CONTENT_COLOR) == 0 &&
	    _cairo_pattern_is_opaque (source, &composite->source_sample_area))
	{
	    pixman_image_unref (r->src);
	    r->src = mask;
	    r->u.mask.src_x = mask_x;
	    r->u.mask.src_y = mask_y;
	    mask = NULL;
	}

	if (mask) {
	    pixman_image_unref (mask);
	    return CAIRO_INT_STATUS_UNSUPPORTED;
	}
    }

    r->u.mask.extents = composite->unbounded;
    r->u.mask.stride = (r->u.mask.extents.width + 3) & ~3;
    if (r->u.mask.extents.height * r->u.mask.stride > (int)sizeof (r->buf)) {
	r->mask = pixman_image_create_bits (PIXMAN_a8,
					    r->u.mask.extents.width,
					    r->u.mask.extents.height,
					    NULL, 0);

	r->base.render_rows = _cairo_image_spans;
	r->base.finish = NULL;
    } else {
	r->mask = pixman_image_create_bits (PIXMAN_a8,
					    r->u.mask.extents.width,
					    r->u.mask.extents.height,
					    (uint32_t *)r->buf, r->u.mask.stride);

	r->base.render_rows = _cairo_image_spans_and_zero;
	r->base.finish = _cairo_image_finish_spans_and_zero;
    }
    if (unlikely (r->mask == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    r->u.mask.data = (uint8_t *) pixman_image_get_data (r->mask);
    r->u.mask.stride = pixman_image_get_stride (r->mask);

    r->u.mask.extents.height += r->u.mask.extents.y;
    return CAIRO_STATUS_SUCCESS;
}