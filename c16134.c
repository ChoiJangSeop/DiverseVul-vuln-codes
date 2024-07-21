rsvg_standard_element_start (RsvgHandle * ctx, const char *name, RsvgPropertyBag * atts)
{

    /*replace this stuff with a hash for fast reading! */
    RsvgNode *newnode = NULL;
    if (!strcmp (name, "g"))
        newnode = rsvg_new_group ();
    else if (!strcmp (name, "a"))       /*treat anchors as groups for now */
        newnode = rsvg_new_group ();
    else if (!strcmp (name, "switch"))
        newnode = rsvg_new_switch ();
    else if (!strcmp (name, "defs"))
        newnode = rsvg_new_defs ();
    else if (!strcmp (name, "use"))
        newnode = rsvg_new_use ();
    else if (!strcmp (name, "path"))
        newnode = rsvg_new_path ();
    else if (!strcmp (name, "line"))
        newnode = rsvg_new_line ();
    else if (!strcmp (name, "rect"))
        newnode = rsvg_new_rect ();
    else if (!strcmp (name, "ellipse"))
        newnode = rsvg_new_ellipse ();
    else if (!strcmp (name, "circle"))
        newnode = rsvg_new_circle ();
    else if (!strcmp (name, "polygon"))
        newnode = rsvg_new_polygon ();
    else if (!strcmp (name, "polyline"))
        newnode = rsvg_new_polyline ();
    else if (!strcmp (name, "symbol"))
        newnode = rsvg_new_symbol ();
    else if (!strcmp (name, "svg"))
        newnode = rsvg_new_svg ();
    else if (!strcmp (name, "mask"))
        newnode = rsvg_new_mask ();
    else if (!strcmp (name, "clipPath"))
        newnode = rsvg_new_clip_path ();
    else if (!strcmp (name, "image"))
        newnode = rsvg_new_image ();
    else if (!strcmp (name, "marker"))
        newnode = rsvg_new_marker ();
    else if (!strcmp (name, "stop"))
        newnode = rsvg_new_stop ();
    else if (!strcmp (name, "pattern"))
        newnode = rsvg_new_pattern ();
    else if (!strcmp (name, "linearGradient"))
        newnode = rsvg_new_linear_gradient ();
    else if (!strcmp (name, "radialGradient"))
        newnode = rsvg_new_radial_gradient ();
    else if (!strcmp (name, "conicalGradient"))
        newnode = rsvg_new_radial_gradient ();
    else if (!strcmp (name, "filter"))
        newnode = rsvg_new_filter ();
    else if (!strcmp (name, "feBlend"))
        newnode = rsvg_new_filter_primitive_blend ();
    else if (!strcmp (name, "feColorMatrix"))
        newnode = rsvg_new_filter_primitive_colour_matrix ();
    else if (!strcmp (name, "feComponentTransfer"))
        newnode = rsvg_new_filter_primitive_component_transfer ();
    else if (!strcmp (name, "feComposite"))
        newnode = rsvg_new_filter_primitive_composite ();
    else if (!strcmp (name, "feConvolveMatrix"))
        newnode = rsvg_new_filter_primitive_convolve_matrix ();
    else if (!strcmp (name, "feDiffuseLighting"))
        newnode = rsvg_new_filter_primitive_diffuse_lighting ();
    else if (!strcmp (name, "feDisplacementMap"))
        newnode = rsvg_new_filter_primitive_displacement_map ();
    else if (!strcmp (name, "feFlood"))
        newnode = rsvg_new_filter_primitive_flood ();
    else if (!strcmp (name, "feGaussianBlur"))
        newnode = rsvg_new_filter_primitive_gaussian_blur ();
    else if (!strcmp (name, "feImage"))
        newnode = rsvg_new_filter_primitive_image ();
    else if (!strcmp (name, "feMerge"))
        newnode = rsvg_new_filter_primitive_merge ();
    else if (!strcmp (name, "feMorphology"))
        newnode = rsvg_new_filter_primitive_erode ();
    else if (!strcmp (name, "feOffset"))
        newnode = rsvg_new_filter_primitive_offset ();
    else if (!strcmp (name, "feSpecularLighting"))
        newnode = rsvg_new_filter_primitive_specular_lighting ();
    else if (!strcmp (name, "feTile"))
        newnode = rsvg_new_filter_primitive_tile ();
    else if (!strcmp (name, "feTurbulence"))
        newnode = rsvg_new_filter_primitive_turbulence ();
    else if (!strcmp (name, "feMergeNode"))
        newnode = rsvg_new_filter_primitive_merge_node ();
    else if (!strcmp (name, "feFuncR"))
        newnode = rsvg_new_node_component_transfer_function ('r');
    else if (!strcmp (name, "feFuncG"))
        newnode = rsvg_new_node_component_transfer_function ('g');
    else if (!strcmp (name, "feFuncB"))
        newnode = rsvg_new_node_component_transfer_function ('b');
    else if (!strcmp (name, "feFuncA"))
        newnode = rsvg_new_node_component_transfer_function ('a');
    else if (!strcmp (name, "feDistantLight"))
        newnode = rsvg_new_filter_primitive_light_source ('d');
    else if (!strcmp (name, "feSpotLight"))
        newnode = rsvg_new_filter_primitive_light_source ('s');
    else if (!strcmp (name, "fePointLight"))
        newnode = rsvg_new_filter_primitive_light_source ('p');
    /* hack to make multiImage sort-of work */
    else if (!strcmp (name, "multiImage"))
        newnode = rsvg_new_switch ();
    else if (!strcmp (name, "subImageRef"))
        newnode = rsvg_new_image ();
    else if (!strcmp (name, "subImage"))
        newnode = rsvg_new_group ();
    else if (!strcmp (name, "text"))
        newnode = rsvg_new_text ();
    else if (!strcmp (name, "tspan"))
        newnode = rsvg_new_tspan ();
    else if (!strcmp (name, "tref"))
        newnode = rsvg_new_tref ();
	else {
		/* hack for bug 401115. whenever we encounter a node we don't understand, push it into a group. 
		   this will allow us to handle things like conditionals properly. */
		newnode = rsvg_new_group ();
	}

    if (newnode) {
        newnode->type = g_string_new (name);
        newnode->parent = ctx->priv->currentnode;
        rsvg_node_set_atts (newnode, ctx, atts);
        rsvg_defs_register_memory (ctx->priv->defs, newnode);
        if (ctx->priv->currentnode) {
            rsvg_node_group_pack (ctx->priv->currentnode, newnode);
            ctx->priv->currentnode = newnode;
        } else if (!strcmp (name, "svg")) {
            ctx->priv->treebase = newnode;
            ctx->priv->currentnode = newnode;
        }
    }
}