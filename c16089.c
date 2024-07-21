_rsvg_node_text_length_children (RsvgNode * self, RsvgDrawingCtx * ctx,
                                 gdouble * x, gboolean * lastwasspace)
{
    guint i;
    int out = FALSE;
    for (i = 0; i < self->children->len; i++) {
        RsvgNode *node = g_ptr_array_index (self->children, i);
        rsvg_state_push (ctx);
        rsvg_state_reinherit_top (ctx, node->state, 0);
        if (!strcmp (node->type->str, "RSVG_NODE_CHARS")) {
            RsvgNodeChars *chars = (RsvgNodeChars *) node;
            GString *str = _rsvg_text_chomp (rsvg_current_state (ctx), chars->contents, lastwasspace);
            *x += rsvg_text_length_text_as_string (ctx, str->str);
            g_string_free (str, TRUE);
        } else if (!strcmp (node->type->str, "tspan")) {
            RsvgNodeText *tspan = (RsvgNodeText *) node;
            out = _rsvg_node_text_length_tspan (tspan, ctx, x, lastwasspace);
        } else if (!strcmp (node->type->str, "tref")) {
            RsvgNodeTref *tref = (RsvgNodeTref *) node;
            out = _rsvg_node_text_length_tref (tref, ctx, x, lastwasspace);
        }
        rsvg_state_pop (ctx);
        if (out)
            break;
    }
    return out;
}