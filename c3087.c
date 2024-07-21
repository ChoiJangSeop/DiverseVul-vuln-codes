static gboolean try_colormap(TGAContext *ctx, GError **err)
{
        TGAColor color;
	guchar *p;
	guint i, n_colors;

	g_return_val_if_fail(ctx != NULL, FALSE);

        if (ctx->cmap_size == 0) {
		g_set_error_literal(err, GDK_PIXBUF_ERROR,
                                    GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                    _("Image is corrupted or truncated"));
			return FALSE;
        }

        n_colors = LE16(ctx->hdr->cmap_n_colors);
        ctx->cmap = colormap_new (n_colors);
	if (!ctx->cmap) {
		g_set_error_literal(err, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                    _("Cannot allocate colormap"));
		return FALSE;
	}

	p = ctx->in->data;
        color.a = 255;

	for (i = 0; i < n_colors; i++) {
		if ((ctx->hdr->cmap_bpp == 15) || (ctx->hdr->cmap_bpp == 16)) {
			guint16 col = p[0] + (p[1] << 8);
			color.b = (col >> 7) & 0xf8;
			color.g = (col >> 2) & 0xf8;
			color.r = col << 3;
			p += 2;
		}
		else if ((ctx->hdr->cmap_bpp == 24) || (ctx->hdr->cmap_bpp == 32)) {
			color.b = *p++;
			color.g = *p++;
			color.r = *p++;
			if (ctx->hdr->cmap_bpp == 32)
				color.a = *p++;
		} else {
			g_set_error_literal(err, GDK_PIXBUF_ERROR, 
                                            GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                            _("Unexpected bitdepth for colormap entries"));
			return FALSE;
		}
                colormap_set_color (ctx->cmap, i, &color);
	}
	ctx->in = io_buffer_free_segment(ctx->in, ctx->cmap_size, err);
	if (!ctx->in)
		return FALSE;
	return TRUE;
}