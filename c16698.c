rsvg_parse_style_pair (RsvgHandle * ctx,
                       RsvgState * state,
                       const gchar * name,
                       const gchar * value,
                       gboolean important)
{
    StyleValueData *data;

    data = g_hash_table_lookup (state->styles, name);
    if (data && data->important && !important)
        return;

    if (name == NULL || value == NULL)
        return;

    g_hash_table_insert (state->styles,
                         (gpointer) g_strdup (name),
                         (gpointer) style_value_data_new (value, important));

    if (g_str_equal (name, "color"))
        state->current_color = rsvg_css_parse_color (value, &state->has_current_color);
    else if (g_str_equal (name, "opacity"))
        state->opacity = rsvg_css_parse_opacity (value);
    else if (g_str_equal (name, "flood-color"))
        state->flood_color = rsvg_css_parse_color (value, &state->has_flood_color);
    else if (g_str_equal (name, "flood-opacity")) {
        state->flood_opacity = rsvg_css_parse_opacity (value);
        state->has_flood_opacity = TRUE;
    } else if (g_str_equal (name, "filter"))
        state->filter = rsvg_filter_parse (ctx->priv->defs, value);
    else if (g_str_equal (name, "a:adobe-blending-mode")) {
        if (g_str_equal (value, "normal"))
            state->adobe_blend = 0;
        else if (g_str_equal (value, "multiply"))
            state->adobe_blend = 1;
        else if (g_str_equal (value, "screen"))
            state->adobe_blend = 2;
        else if (g_str_equal (value, "darken"))
            state->adobe_blend = 3;
        else if (g_str_equal (value, "lighten"))
            state->adobe_blend = 4;
        else if (g_str_equal (value, "softlight"))
            state->adobe_blend = 5;
        else if (g_str_equal (value, "hardlight"))
            state->adobe_blend = 6;
        else if (g_str_equal (value, "colordodge"))
            state->adobe_blend = 7;
        else if (g_str_equal (value, "colorburn"))
            state->adobe_blend = 8;
        else if (g_str_equal (value, "overlay"))
            state->adobe_blend = 9;
        else if (g_str_equal (value, "exclusion"))
            state->adobe_blend = 10;
        else if (g_str_equal (value, "difference"))
            state->adobe_blend = 11;
        else
            state->adobe_blend = 0;
    } else if (g_str_equal (name, "mask"))
        state->mask = rsvg_mask_parse (ctx->priv->defs, value);
    else if (g_str_equal (name, "clip-path")) {
        state->clip_path_ref = rsvg_clip_path_parse (ctx->priv->defs, value);
    } else if (g_str_equal (name, "overflow")) {
        if (!g_str_equal (value, "inherit")) {
            state->overflow = rsvg_css_parse_overflow (value, &state->has_overflow);
        }
    } else if (g_str_equal (name, "enable-background")) {
        if (g_str_equal (value, "new"))
            state->enable_background = RSVG_ENABLE_BACKGROUND_NEW;
        else
            state->enable_background = RSVG_ENABLE_BACKGROUND_ACCUMULATE;
    } else if (g_str_equal (name, "comp-op")) {
        if (g_str_equal (value, "clear"))
            state->comp_op = CAIRO_OPERATOR_CLEAR;
        else if (g_str_equal (value, "src"))
            state->comp_op = CAIRO_OPERATOR_SOURCE;
        else if (g_str_equal (value, "dst"))
            state->comp_op = CAIRO_OPERATOR_DEST;
        else if (g_str_equal (value, "src-over"))
            state->comp_op = CAIRO_OPERATOR_OVER;
        else if (g_str_equal (value, "dst-over"))
            state->comp_op = CAIRO_OPERATOR_DEST_OVER;
        else if (g_str_equal (value, "src-in"))
            state->comp_op = CAIRO_OPERATOR_IN;
        else if (g_str_equal (value, "dst-in"))
            state->comp_op = CAIRO_OPERATOR_DEST_IN;
        else if (g_str_equal (value, "src-out"))
            state->comp_op = CAIRO_OPERATOR_OUT;
        else if (g_str_equal (value, "dst-out"))
            state->comp_op = CAIRO_OPERATOR_DEST_OUT;
        else if (g_str_equal (value, "src-atop"))
            state->comp_op = CAIRO_OPERATOR_ATOP;
        else if (g_str_equal (value, "dst-atop"))
            state->comp_op = CAIRO_OPERATOR_DEST_ATOP;
        else if (g_str_equal (value, "xor"))
            state->comp_op = CAIRO_OPERATOR_XOR;
        else if (g_str_equal (value, "plus"))
            state->comp_op = CAIRO_OPERATOR_ADD;
        else if (g_str_equal (value, "multiply"))
            state->comp_op = CAIRO_OPERATOR_MULTIPLY;
        else if (g_str_equal (value, "screen"))
            state->comp_op = CAIRO_OPERATOR_SCREEN;
        else if (g_str_equal (value, "overlay"))
            state->comp_op = CAIRO_OPERATOR_OVERLAY;
        else if (g_str_equal (value, "darken"))
            state->comp_op = CAIRO_OPERATOR_DARKEN;
        else if (g_str_equal (value, "lighten"))
            state->comp_op = CAIRO_OPERATOR_LIGHTEN;
        else if (g_str_equal (value, "color-dodge"))
            state->comp_op = CAIRO_OPERATOR_COLOR_DODGE;
        else if (g_str_equal (value, "color-burn"))
            state->comp_op = CAIRO_OPERATOR_COLOR_BURN;
        else if (g_str_equal (value, "hard-light"))
            state->comp_op = CAIRO_OPERATOR_HARD_LIGHT;
        else if (g_str_equal (value, "soft-light"))
            state->comp_op = CAIRO_OPERATOR_SOFT_LIGHT;
        else if (g_str_equal (value, "difference"))
            state->comp_op = CAIRO_OPERATOR_DIFFERENCE;
        else if (g_str_equal (value, "exclusion"))
            state->comp_op = CAIRO_OPERATOR_EXCLUSION;
        else
            state->comp_op = CAIRO_OPERATOR_OVER;
    } else if (g_str_equal (name, "display")) {
        state->has_visible = TRUE;
        if (g_str_equal (value, "none"))
            state->visible = FALSE;
        else if (!g_str_equal (value, "inherit") != 0)
            state->visible = TRUE;
        else
            state->has_visible = FALSE;
	} else if (g_str_equal (name, "xml:space")) {
        state->has_space_preserve = TRUE;
        if (g_str_equal (value, "default"))
            state->space_preserve = FALSE;
        else if (!g_str_equal (value, "preserve") == 0)
            state->space_preserve = TRUE;
        else
            state->space_preserve = FALSE;
    } else if (g_str_equal (name, "visibility")) {
        state->has_visible = TRUE;
        if (g_str_equal (value, "visible"))
            state->visible = TRUE;
        else if (!g_str_equal (value, "inherit") != 0)
            state->visible = FALSE;     /* collapse or hidden */
        else
            state->has_visible = FALSE;
    } else if (g_str_equal (name, "fill")) {
        RsvgPaintServer *fill = state->fill;
        state->fill =
            rsvg_paint_server_parse (&state->has_fill_server, ctx->priv->defs, value, 0);
        rsvg_paint_server_unref (fill);
    } else if (g_str_equal (name, "fill-opacity")) {
        state->fill_opacity = rsvg_css_parse_opacity (value);
        state->has_fill_opacity = TRUE;
    } else if (g_str_equal (name, "fill-rule")) {
        state->has_fill_rule = TRUE;
        if (g_str_equal (value, "nonzero"))
            state->fill_rule = CAIRO_FILL_RULE_WINDING;
        else if (g_str_equal (value, "evenodd"))
            state->fill_rule = CAIRO_FILL_RULE_EVEN_ODD;
        else
            state->has_fill_rule = FALSE;
    } else if (g_str_equal (name, "clip-rule")) {
        state->has_clip_rule = TRUE;
        if (g_str_equal (value, "nonzero"))
            state->clip_rule = CAIRO_FILL_RULE_WINDING;
        else if (g_str_equal (value, "evenodd"))
            state->clip_rule = CAIRO_FILL_RULE_EVEN_ODD;
        else
            state->has_clip_rule = FALSE;
    } else if (g_str_equal (name, "stroke")) {
        RsvgPaintServer *stroke = state->stroke;

        state->stroke =
            rsvg_paint_server_parse (&state->has_stroke_server, ctx->priv->defs, value, 0);

        rsvg_paint_server_unref (stroke);
    } else if (g_str_equal (name, "stroke-width")) {
        state->stroke_width = _rsvg_css_parse_length (value);
        state->has_stroke_width = TRUE;
    } else if (g_str_equal (name, "stroke-linecap")) {
        state->has_cap = TRUE;
        if (g_str_equal (value, "butt"))
            state->cap = CAIRO_LINE_CAP_BUTT;
        else if (g_str_equal (value, "round"))
            state->cap = CAIRO_LINE_CAP_ROUND;
        else if (g_str_equal (value, "square"))
            state->cap = CAIRO_LINE_CAP_SQUARE;
        else
            g_warning (_("unknown line cap style %s\n"), value);
    } else if (g_str_equal (name, "stroke-opacity")) {
        state->stroke_opacity = rsvg_css_parse_opacity (value);
        state->has_stroke_opacity = TRUE;
    } else if (g_str_equal (name, "stroke-linejoin")) {
        state->has_join = TRUE;
        if (g_str_equal (value, "miter"))
            state->join = CAIRO_LINE_JOIN_MITER;
        else if (g_str_equal (value, "round"))
            state->join = CAIRO_LINE_JOIN_ROUND;
        else if (g_str_equal (value, "bevel"))
            state->join = CAIRO_LINE_JOIN_BEVEL;
        else
            g_warning (_("unknown line join style %s\n"), value);
    } else if (g_str_equal (name, "font-size")) {
        state->font_size = _rsvg_css_parse_length (value);
        state->has_font_size = TRUE;
    } else if (g_str_equal (name, "font-family")) {
        char *save = g_strdup (rsvg_css_parse_font_family (value, &state->has_font_family));
        g_free (state->font_family);
        state->font_family = save;
    } else if (g_str_equal (name, "xml:lang")) {
        char *save = g_strdup (value);
        g_free (state->lang);
        state->lang = save;
        state->has_lang = TRUE;
    } else if (g_str_equal (name, "font-style")) {
        state->font_style = rsvg_css_parse_font_style (value, &state->has_font_style);
    } else if (g_str_equal (name, "font-variant")) {
        state->font_variant = rsvg_css_parse_font_variant (value, &state->has_font_variant);
    } else if (g_str_equal (name, "font-weight")) {
        state->font_weight = rsvg_css_parse_font_weight (value, &state->has_font_weight);
    } else if (g_str_equal (name, "font-stretch")) {
        state->font_stretch = rsvg_css_parse_font_stretch (value, &state->has_font_stretch);
    } else if (g_str_equal (name, "text-decoration")) {
        if (g_str_equal (value, "inherit")) {
            state->has_font_decor = FALSE;
            state->font_decor = TEXT_NORMAL;
        } else {
            if (strstr (value, "underline"))
                state->font_decor |= TEXT_UNDERLINE;
            if (strstr (value, "overline"))
                state->font_decor |= TEXT_OVERLINE;
            if (strstr (value, "strike") || strstr (value, "line-through"))     /* strike though or line-through */
                state->font_decor |= TEXT_STRIKE;
            state->has_font_decor = TRUE;
        }
    } else if (g_str_equal (name, "direction")) {
        state->has_text_dir = TRUE;
        if (g_str_equal (value, "inherit")) {
            state->text_dir = PANGO_DIRECTION_LTR;
            state->has_text_dir = FALSE;
        } else if (g_str_equal (value, "rtl"))
            state->text_dir = PANGO_DIRECTION_RTL;
        else                    /* ltr */
            state->text_dir = PANGO_DIRECTION_LTR;
    } else if (g_str_equal (name, "unicode-bidi")) {
        state->has_unicode_bidi = TRUE;
        if (g_str_equal (value, "inherit")) {
            state->unicode_bidi = UNICODE_BIDI_NORMAL;
            state->has_unicode_bidi = FALSE;
        } else if (g_str_equal (value, "embed"))
            state->unicode_bidi = UNICODE_BIDI_EMBED;
        else if (g_str_equal (value, "bidi-override"))
            state->unicode_bidi = UNICODE_BIDI_OVERRIDE;
        else                    /* normal */
            state->unicode_bidi = UNICODE_BIDI_NORMAL;
    } else if (g_str_equal (name, "writing-mode")) {
        /* TODO: these aren't quite right... */

        state->has_text_dir = TRUE;
        state->has_text_gravity = TRUE;
        if (g_str_equal (value, "inherit")) {
            state->text_dir = PANGO_DIRECTION_LTR;
            state->has_text_dir = FALSE;
            state->text_gravity = PANGO_GRAVITY_SOUTH;
            state->has_text_gravity = FALSE;
        } else if (g_str_equal (value, "lr-tb") || g_str_equal (value, "lr")) {
            state->text_dir = PANGO_DIRECTION_LTR;
            state->text_gravity = PANGO_GRAVITY_SOUTH;
        } else if (g_str_equal (value, "rl-tb") || g_str_equal (value, "rl")) {
            state->text_dir = PANGO_DIRECTION_RTL;
            state->text_gravity = PANGO_GRAVITY_SOUTH;
        } else if (g_str_equal (value, "tb-rl") || g_str_equal (value, "tb")) {
            state->text_dir = PANGO_DIRECTION_LTR;
            state->text_gravity = PANGO_GRAVITY_EAST;
        }
    } else if (g_str_equal (name, "text-anchor")) {
        state->has_text_anchor = TRUE;
        if (g_str_equal (value, "inherit")) {
            state->text_anchor = TEXT_ANCHOR_START;
            state->has_text_anchor = FALSE;
        } else {
            if (strstr (value, "start"))
                state->text_anchor = TEXT_ANCHOR_START;
            else if (strstr (value, "middle"))
                state->text_anchor = TEXT_ANCHOR_MIDDLE;
            else if (strstr (value, "end"))
                state->text_anchor = TEXT_ANCHOR_END;
        }
    } else if (g_str_equal (name, "letter-spacing")) {
	state->has_letter_spacing = TRUE;
	state->letter_spacing = _rsvg_css_parse_length (value);
    } else if (g_str_equal (name, "stop-color")) {
        if (!g_str_equal (value, "inherit")) {
            state->stop_color = rsvg_css_parse_color (value, &state->has_stop_color);
        }
    } else if (g_str_equal (name, "stop-opacity")) {
        if (!g_str_equal (value, "inherit")) {
            state->has_stop_opacity = TRUE;
            state->stop_opacity = rsvg_css_parse_opacity (value);
        }
    } else if (g_str_equal (name, "marker-start")) {
        state->startMarker = rsvg_marker_parse (ctx->priv->defs, value);
        state->has_startMarker = TRUE;
    } else if (g_str_equal (name, "marker-mid")) {
        state->middleMarker = rsvg_marker_parse (ctx->priv->defs, value);
        state->has_middleMarker = TRUE;
    } else if (g_str_equal (name, "marker-end")) {
        state->endMarker = rsvg_marker_parse (ctx->priv->defs, value);
        state->has_endMarker = TRUE;
    } else if (g_str_equal (name, "stroke-miterlimit")) {
        state->has_miter_limit = TRUE;
        state->miter_limit = g_ascii_strtod (value, NULL);
    } else if (g_str_equal (name, "stroke-dashoffset")) {
        state->has_dashoffset = TRUE;
        state->dash.offset = _rsvg_css_parse_length (value);
        if (state->dash.offset.length < 0.)
            state->dash.offset.length = 0.;
    } else if (g_str_equal (name, "shape-rendering")) {
        state->has_shape_rendering_type = TRUE;

        if (g_str_equal (value, "auto") || g_str_equal (value, "default"))
            state->shape_rendering_type = SHAPE_RENDERING_AUTO;
        else if (g_str_equal (value, "optimizeSpeed"))
            state->shape_rendering_type = SHAPE_RENDERING_OPTIMIZE_SPEED;
        else if (g_str_equal (value, "crispEdges"))
            state->shape_rendering_type = SHAPE_RENDERING_CRISP_EDGES;
        else if (g_str_equal (value, "geometricPrecision"))
            state->shape_rendering_type = SHAPE_RENDERING_GEOMETRIC_PRECISION;

    } else if (g_str_equal (name, "text-rendering")) {
        state->has_text_rendering_type = TRUE;

        if (g_str_equal (value, "auto") || g_str_equal (value, "default"))
            state->text_rendering_type = TEXT_RENDERING_AUTO;
        else if (g_str_equal (value, "optimizeSpeed"))
            state->text_rendering_type = TEXT_RENDERING_OPTIMIZE_SPEED;
        else if (g_str_equal (value, "optimizeLegibility"))
            state->text_rendering_type = TEXT_RENDERING_OPTIMIZE_LEGIBILITY;
        else if (g_str_equal (value, "geometricPrecision"))
            state->text_rendering_type = TEXT_RENDERING_GEOMETRIC_PRECISION;

    } else if (g_str_equal (name, "stroke-dasharray")) {
        state->has_dash = TRUE;
        if (g_str_equal (value, "none")) {
            if (state->dash.n_dash != 0) {
                /* free any cloned dash data */
                g_free (state->dash.dash);
                state->dash.dash = NULL;
                state->dash.n_dash = 0;
            }
        } else {
            gchar **dashes = g_strsplit (value, ",", -1);
            if (NULL != dashes) {
                gint n_dashes, i;
                gboolean is_even = FALSE;
                gdouble total = 0;

                /* count the #dashes */
                for (n_dashes = 0; dashes[n_dashes] != NULL; n_dashes++);

                is_even = (n_dashes % 2 == 0);
                state->dash.n_dash = (is_even ? n_dashes : n_dashes * 2);
                state->dash.dash = g_new (double, state->dash.n_dash);

                /* TODO: handle negative value == error case */

                /* the even and base case */
                for (i = 0; i < n_dashes; i++) {
                    state->dash.dash[i] = g_ascii_strtod (dashes[i], NULL);
                    total += state->dash.dash[i];
                }
                /* if an odd number of dashes is found, it gets repeated */
                if (!is_even)
                    for (; i < state->dash.n_dash; i++)
                        state->dash.dash[i] = state->dash.dash[i - n_dashes];

                g_strfreev (dashes);
                /* If the dashes add up to 0, then it should 
                   be ignored */
                if (total == 0) {
                    g_free (state->dash.dash);
                    state->dash.dash = NULL;
                    state->dash.n_dash = 0;
                }
            }
        }
    }
}