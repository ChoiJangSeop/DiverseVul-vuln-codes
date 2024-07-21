rsvg_state_inherit_run (RsvgState * dst, const RsvgState * src,
                        const InheritanceFunction function, const gboolean inherituninheritables)
{
    gint i;

    if (function (dst->has_current_color, src->has_current_color))
        dst->current_color = src->current_color;
    if (function (dst->has_flood_color, src->has_flood_color))
        dst->flood_color = src->flood_color;
    if (function (dst->has_flood_opacity, src->has_flood_opacity))
        dst->flood_opacity = src->flood_opacity;
    if (function (dst->has_fill_server, src->has_fill_server)) {
        rsvg_paint_server_ref (src->fill);
        if (dst->fill)
            rsvg_paint_server_unref (dst->fill);
        dst->fill = src->fill;
    }
    if (function (dst->has_fill_opacity, src->has_fill_opacity))
        dst->fill_opacity = src->fill_opacity;
    if (function (dst->has_fill_rule, src->has_fill_rule))
        dst->fill_rule = src->fill_rule;
    if (function (dst->has_clip_rule, src->has_clip_rule))
        dst->clip_rule = src->clip_rule;
    if (function (dst->overflow, src->overflow))
        dst->overflow = src->overflow;
    if (function (dst->has_stroke_server, src->has_stroke_server)) {
        rsvg_paint_server_ref (src->stroke);
        if (dst->stroke)
            rsvg_paint_server_unref (dst->stroke);
        dst->stroke = src->stroke;
    }
    if (function (dst->has_stroke_opacity, src->has_stroke_opacity))
        dst->stroke_opacity = src->stroke_opacity;
    if (function (dst->has_stroke_width, src->has_stroke_width))
        dst->stroke_width = src->stroke_width;
    if (function (dst->has_miter_limit, src->has_miter_limit))
        dst->miter_limit = src->miter_limit;
    if (function (dst->has_cap, src->has_cap))
        dst->cap = src->cap;
    if (function (dst->has_join, src->has_join))
        dst->join = src->join;
    if (function (dst->has_stop_color, src->has_stop_color))
        dst->stop_color = src->stop_color;
    if (function (dst->has_stop_opacity, src->has_stop_opacity))
        dst->stop_opacity = src->stop_opacity;
    if (function (dst->has_cond, src->has_cond))
        dst->cond_true = src->cond_true;
    if (function (dst->has_font_size, src->has_font_size))
        dst->font_size = src->font_size;
    if (function (dst->has_font_style, src->has_font_style))
        dst->font_style = src->font_style;
    if (function (dst->has_font_variant, src->has_font_variant))
        dst->font_variant = src->font_variant;
    if (function (dst->has_font_weight, src->has_font_weight))
        dst->font_weight = src->font_weight;
    if (function (dst->has_font_stretch, src->has_font_stretch))
        dst->font_stretch = src->font_stretch;
    if (function (dst->has_font_decor, src->has_font_decor))
        dst->font_decor = src->font_decor;
    if (function (dst->has_text_dir, src->has_text_dir))
        dst->text_dir = src->text_dir;
    if (function (dst->has_text_gravity, src->has_text_gravity))
        dst->text_gravity = src->text_gravity;
    if (function (dst->has_unicode_bidi, src->has_unicode_bidi))
        dst->unicode_bidi = src->unicode_bidi;
    if (function (dst->has_text_anchor, src->has_text_anchor))
        dst->text_anchor = src->text_anchor;
    if (function (dst->has_letter_spacing, src->has_letter_spacing))
        dst->letter_spacing = src->letter_spacing;
    if (function (dst->has_startMarker, src->has_startMarker))
        dst->startMarker = src->startMarker;
    if (function (dst->has_middleMarker, src->has_middleMarker))
        dst->middleMarker = src->middleMarker;
    if (function (dst->has_endMarker, src->has_endMarker))
        dst->endMarker = src->endMarker;
	if (function (dst->has_shape_rendering_type, src->has_shape_rendering_type))
		dst->shape_rendering_type = src->shape_rendering_type;
	if (function (dst->has_text_rendering_type, src->has_text_rendering_type))
		dst->text_rendering_type = src->text_rendering_type;

    if (function (dst->has_font_family, src->has_font_family)) {
        g_free (dst->font_family);      /* font_family is always set to something */
        dst->font_family = g_strdup (src->font_family);
    }

    if (function (dst->has_space_preserve, src->has_space_preserve))
        dst->space_preserve = src->space_preserve;

    if (function (dst->has_visible, src->has_visible))
        dst->visible = src->visible;

    if (function (dst->has_lang, src->has_lang)) {
        if (dst->has_lang)
            g_free (dst->lang);
        dst->lang = g_strdup (src->lang);
    }

    if (src->dash.n_dash > 0 && (function (dst->has_dash, src->has_dash))) {
        if (dst->has_dash)
            g_free (dst->dash.dash);

        dst->dash.dash = g_new (gdouble, src->dash.n_dash);
        dst->dash.n_dash = src->dash.n_dash;
        for (i = 0; i < src->dash.n_dash; i++)
            dst->dash.dash[i] = src->dash.dash[i];
    }

    if (function (dst->has_dashoffset, src->has_dashoffset)) {
        dst->dash.offset = src->dash.offset;
    }

    if (inherituninheritables) {
        dst->clip_path_ref = src->clip_path_ref;
        dst->mask = src->mask;
        dst->enable_background = src->enable_background;
        dst->adobe_blend = src->adobe_blend;
        dst->opacity = src->opacity;
        dst->filter = src->filter;
        dst->comp_op = src->comp_op;
    }
}