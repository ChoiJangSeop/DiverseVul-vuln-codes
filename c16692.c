rsvg_cairo_push_render_stack (RsvgDrawingCtx * ctx)
{
    /* XXX: Untested, probably needs help wrt filters */

    RsvgCairoRender *render = RSVG_CAIRO_RENDER (ctx->render);
    cairo_surface_t *surface;
    cairo_t *child_cr;
    RsvgBbox *bbox;
    RsvgState *state = rsvg_current_state (ctx);
    gboolean lateclip = FALSE;

    if (rsvg_current_state (ctx)->clip_path) {
        RsvgNode *node;
        node = rsvg_defs_lookup (ctx->defs, rsvg_current_state (ctx)->clip_path);
        if (node && RSVG_NODE_TYPE (node) == RSVG_NODE_TYPE_CLIP_PATH) {
            RsvgClipPath *clip_path = (RsvgClipPath *) node;

            switch (clip_path->units) {
            case userSpaceOnUse:
                rsvg_cairo_clip (ctx, clip_path, NULL);
                break;
            case objectBoundingBox:
                lateclip = TRUE;
                break;

            default:
                g_assert_not_reached ();
                break;
            }

        }
    }

    if (state->opacity == 0xFF
        && !state->filter && !state->mask && !lateclip && (state->comp_op == CAIRO_OPERATOR_OVER)
        && (state->enable_background == RSVG_ENABLE_BACKGROUND_ACCUMULATE))
        return;

    if (!state->filter) {
        surface = cairo_surface_create_similar (cairo_get_target (render->cr),
                                                CAIRO_CONTENT_COLOR_ALPHA,
                                                render->width, render->height);
    } else {
        surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 
                                              render->width, render->height);

        /* The surface reference is owned by the child_cr created below and put on the cr_stack! */
        render->surfaces_stack = g_list_prepend (render->surfaces_stack, surface);
    }

#if 0
    if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy (surface);
        return;
    }
#endif

    child_cr = cairo_create (surface);
    cairo_surface_destroy (surface);

    render->cr_stack = g_list_prepend (render->cr_stack, render->cr);
    render->cr = child_cr;

    bbox = g_new (RsvgBbox, 1);
    *bbox = render->bbox;
    render->bb_stack = g_list_prepend (render->bb_stack, bbox);
    rsvg_bbox_init (&render->bbox, &state->affine);
}