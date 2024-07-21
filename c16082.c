_rsvg_node_text_type_children (RsvgNode * self, RsvgDrawingCtx * ctx,
                               gdouble * x, gdouble * y, gboolean * lastwasspace)
{
    guint i;

    rsvg_push_discrete_layer (ctx);
    for (i = 0; i < self->children->len; i++) {
        RsvgNode *node = g_ptr_array_index (self->children, i);
        if (!strcmp (node->type->str, "RSVG_NODE_CHARS")) {
            RsvgNodeChars *chars = (RsvgNodeChars *) node;
            GString *str = _rsvg_text_chomp (rsvg_current_state (ctx), chars->contents, lastwasspace);
            rsvg_text_render_text (ctx, str->str, x, y);
            g_string_free (str, TRUE);
        } else if (!strcmp (node->type->str, "tspan")) {
            RsvgNodeText *tspan = (RsvgNodeText *) node;
            rsvg_state_push (ctx);
            _rsvg_node_text_type_tspan (tspan, ctx, x, y, lastwasspace);
            rsvg_state_pop (ctx);
        } else if (!strcmp (node->type->str, "tref")) {
            RsvgNodeTref *tref = (RsvgNodeTref *) node;
            _rsvg_node_text_type_tref (tref, ctx, x, y, lastwasspace);
        }
    }
    rsvg_pop_discrete_layer (ctx);
}