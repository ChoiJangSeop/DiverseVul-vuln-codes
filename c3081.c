static gboolean try_colormap(TGAContext *ctx, GError **err)
{
	static guchar *p;
	static guint n;

	g_return_val_if_fail(ctx != NULL, FALSE);

        if (ctx->cmap_size == 0) {
		g_set_error_literal(err, GDK_PIXBUF_ERROR,
                                    GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                    _("Image is corrupted or truncated"));
			return FALSE;
        }

	ctx->cmap = g_try_malloc(sizeof(TGAColormap));
	if (!ctx->cmap) {
		g_set_error_literal(err, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                    _("Cannot allocate colormap structure"));
		return FALSE;
	}
	ctx->cmap->size = LE16(ctx->hdr->cmap_n_colors);
	ctx->cmap->cols = g_try_malloc(sizeof(TGAColor) * ctx->cmap->size);
	if (!ctx->cmap->cols) {
		g_set_error_literal(err, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                    _("Cannot allocate colormap entries"));
		return FALSE;
	}

	p = ctx->in->data;
	for (n = 0; n < ctx->cmap->size; n++) {
		if ((ctx->hdr->cmap_bpp == 15) || (ctx->hdr->cmap_bpp == 16)) {
			guint16 col = p[0] + (p[1] << 8);
			ctx->cmap->cols[n].b = (col >> 7) & 0xf8;
			ctx->cmap->cols[n].g = (col >> 2) & 0xf8;
			ctx->cmap->cols[n].r = col << 3;
			p += 2;
		}
		else if ((ctx->hdr->cmap_bpp == 24) || (ctx->hdr->cmap_bpp == 32)) {
			ctx->cmap->cols[n].b = *p++;
			ctx->cmap->cols[n].g = *p++;
			ctx->cmap->cols[n].r = *p++;
			if (ctx->hdr->cmap_bpp == 32)
				ctx->cmap->cols[n].a = *p++;
		} else {
			g_set_error_literal(err, GDK_PIXBUF_ERROR, 
                                            GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                            _("Unexpected bitdepth for colormap entries"));
			return FALSE;
		}
	}
	ctx->in = io_buffer_free_segment(ctx->in, ctx->cmap_size, err);
	if (!ctx->in)
		return FALSE;
	return TRUE;
}