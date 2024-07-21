gerbv_gdk_draw_amacro(GdkPixmap *pixmap, GdkGC *gc, 
		      gerbv_simplified_amacro_t *s, double scale, 
		      gint x, gint y)
{
	dprintf("%s(): drawing simplified aperture macros:\n", __func__);

	while (s != NULL) {
		if (s->type >= GERBV_APTYPE_MACRO_CIRCLE
		 && s->type <= GERBV_APTYPE_MACRO_LINE22) {
			dgk_draw_amacro_funcs[s->type](pixmap, gc,
					s->parameter, scale, x, y);
			dprintf("  %s\n", gerbv_aperture_type_name(s->type));
		} else {
			GERB_FATAL_ERROR(
				_("Unknown simplified aperture macro type %d"),
				s->type);
		}

		s = s->next;
	}
} /* gerbv_gdk_draw_amacro */