rsvg_node_use_draw (RsvgNode * self, RsvgDrawingCtx * ctx, int dominate)
{
    RsvgNodeUse *use = (RsvgNodeUse *) self;
    RsvgNode *child;
    RsvgState *state;
    double affine[6];
    double x, y, w, h;
    x = _rsvg_css_normalize_length (&use->x, ctx, 'h');
    y = _rsvg_css_normalize_length (&use->y, ctx, 'v');
    w = _rsvg_css_normalize_length (&use->w, ctx, 'h');
    h = _rsvg_css_normalize_length (&use->h, ctx, 'v');

    rsvg_state_reinherit_top (ctx, self->state, dominate);

    child = use->link;

    /* If it can find nothing to draw... draw nothing */
    if (!child)
        return;
    else if (rsvg_node_is_ancestor (child, self))       /* or, if we're <use>'ing ourself */
        return;

    state = rsvg_current_state (ctx);
    if (strcmp (child->type->str, "symbol")) {
        _rsvg_affine_translate (affine, x, y);
        _rsvg_affine_multiply (state->affine, affine, state->affine);

        rsvg_push_discrete_layer (ctx);
        rsvg_state_push (ctx);
        rsvg_node_draw (child, ctx, 1);
        rsvg_state_pop (ctx);
        rsvg_pop_discrete_layer (ctx);
    } else {
        RsvgNodeSymbol *symbol = (RsvgNodeSymbol *) child;

        if (symbol->vbox.active) {
            rsvg_preserve_aspect_ratio
                (symbol->preserve_aspect_ratio, symbol->vbox.w, symbol->vbox.h, &w, &h, &x, &y);

            _rsvg_affine_translate (affine, x, y);
            _rsvg_affine_multiply (state->affine, affine, state->affine);
            _rsvg_affine_scale (affine, w / symbol->vbox.w, h / symbol->vbox.h);
            _rsvg_affine_multiply (state->affine, affine, state->affine);
            _rsvg_affine_translate (affine, -symbol->vbox.x, -symbol->vbox.y);
            _rsvg_affine_multiply (state->affine, affine, state->affine);

            _rsvg_push_view_box (ctx, symbol->vbox.w, symbol->vbox.h);
            rsvg_push_discrete_layer (ctx);
            if (!state->overflow || (!state->has_overflow && child->state->overflow))
                rsvg_add_clipping_rect (ctx, symbol->vbox.x, symbol->vbox.y,
                                        symbol->vbox.w, symbol->vbox.h);
        } else {
            _rsvg_affine_translate (affine, x, y);
            _rsvg_affine_multiply (state->affine, affine, state->affine);
            rsvg_push_discrete_layer (ctx);
        }

        rsvg_state_push (ctx);
        _rsvg_node_draw_children (child, ctx, 1);
        rsvg_state_pop (ctx);
        rsvg_pop_discrete_layer (ctx);
        if (symbol->vbox.active)
            _rsvg_pop_view_box (ctx);
    }

}