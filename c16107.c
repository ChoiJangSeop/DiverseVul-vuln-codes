_rsvg_node_poly_set_atts (RsvgNode * self, RsvgHandle * ctx, RsvgPropertyBag * atts)
{
    RsvgNodePoly *poly = (RsvgNodePoly *) self;
    const char *klazz = NULL, *id = NULL, *value;

    if (rsvg_property_bag_size (atts)) {
        /* support for svg < 1.0 which used verts */
        if ((value = rsvg_property_bag_lookup (atts, "verts"))
            || (value = rsvg_property_bag_lookup (atts, "points"))) {
            poly->pointlist = rsvg_css_parse_number_list (value, &poly->pointlist_len);
        }
        if ((value = rsvg_property_bag_lookup (atts, "class")))
            klazz = value;
        if ((value = rsvg_property_bag_lookup (atts, "id"))) {
            id = value;
            rsvg_defs_register_name (ctx->priv->defs, value, self);
        }

        rsvg_parse_style_attrs (ctx, self->state, (poly->is_polyline ? "polyline" : "polygon"),
                                klazz, id, atts);
    }

}