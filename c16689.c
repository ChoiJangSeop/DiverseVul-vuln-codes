rsvg_cairo_pop_render_stack (RsvgDrawingCtx * ctx)
{
    RsvgCairoRender *render = RSVG_CAIRO_RENDER (ctx->render);
    cairo_t *child_cr = render->cr;
    RsvgClipPath *lateclip = NULL;
    cairo_surface_t *surface = NULL;
    RsvgState *state = rsvg_current_state (ctx);
    gboolean nest;

    if (rsvg_current_state (ctx)->clip_path) {
        RsvgNode *node;
        node = rsvg_defs_lookup (ctx->defs, rsvg_current_state (ctx)->clip_path);
        if (node && RSVG_NODE_TYPE (node) == RSVG_NODE_TYPE_CLIP_PATH
            && ((RsvgClipPath *) node)->units == objectBoundingBox)
            lateclip = (RsvgClipPath *) node;
    }

    if (state->opacity == 0xFF
        && !state->filter && !state->mask && !lateclip && (state->comp_op == CAIRO_OPERATOR_OVER)
        && (state->enable_background == RSVG_ENABLE_BACKGROUND_ACCUMULATE))
        return;

    if (state->filter) {
        cairo_surface_t *output;

        output = render->surfaces_stack->data;
        render->surfaces_stack = g_list_delete_link (render->surfaces_stack, render->surfaces_stack);

        surface = rsvg_filter_render (state->filter, output, ctx, &render->bbox, "2103");

        /* Don't destroy the output surface, it's owned by child_cr */
    } else {
        surface = cairo_get_target (child_cr);
    }

    render->cr = (cairo_t *) render->cr_stack->data;
    render->cr_stack = g_list_delete_link (render->cr_stack, render->cr_stack);

    nest = render->cr != render->initial_cr;
    cairo_identity_matrix (render->cr);
    cairo_set_source_surface (render->cr, surface,
                              nest ? 0 : render->offset_x,
                              nest ? 0 : render->offset_y);

    if (lateclip)
        rsvg_cairo_clip (ctx, lateclip, &render->bbox);

    cairo_set_operator (render->cr, state->comp_op);

    if (state->mask) {
        RsvgNode *mask;

        mask = rsvg_defs_lookup (ctx->defs, state->mask);
        if (mask && RSVG_NODE_TYPE (mask) == RSVG_NODE_TYPE_MASK)
          rsvg_cairo_generate_mask (render->cr, (RsvgMask *) mask, ctx, &render->bbox);
    } else if (state->opacity != 0xFF)
        cairo_paint_with_alpha (render->cr, (double) state->opacity / 255.0);
    else
        cairo_paint (render->cr);

    cairo_destroy (child_cr);

    rsvg_bbox_insert ((RsvgBbox *) render->bb_stack->data, &render->bbox);

    render->bbox = *((RsvgBbox *) render->bb_stack->data);

    g_free (render->bb_stack->data);
    render->bb_stack = g_list_delete_link (render->bb_stack, render->bb_stack);

    if (state->filter) {
        cairo_surface_destroy (surface);
    }
}