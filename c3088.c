static gboolean try_preload(TGAContext *ctx, GError **err)
{
	if (!ctx->hdr) {
		if (ctx->in->size >= sizeof(TGAHeader)) {
			ctx->hdr = g_try_malloc(sizeof(TGAHeader));
			if (!ctx->hdr) {
				g_set_error_literal(err, GDK_PIXBUF_ERROR,
                                                    GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                                    _("Cannot allocate TGA header memory"));
				return FALSE;
			}
			g_memmove(ctx->hdr, ctx->in->data, sizeof(TGAHeader));
			ctx->in = io_buffer_free_segment(ctx->in, sizeof(TGAHeader), err);
#ifdef DEBUG_TGA
			g_print ("infolen %d "
				 "has_cmap %d "
				 "type %d "
				 "cmap_start %d "
				 "cmap_n_colors %d "
				 "cmap_bpp %d "
				 "x %d y %d width %d height %d bpp %d "
				 "flags %#x",
				 ctx->hdr->infolen,
				 ctx->hdr->has_cmap,
				 ctx->hdr->type,
				 LE16(ctx->hdr->cmap_start),
				 LE16(ctx->hdr->cmap_n_colors),
				 ctx->hdr->cmap_bpp,
				 LE16(ctx->hdr->x_origin),
				 LE16(ctx->hdr->y_origin),
				 LE16(ctx->hdr->width),
				 LE16(ctx->hdr->height),
				 ctx->hdr->bpp,
				 ctx->hdr->flags);
#endif
			if (!ctx->in)
				return FALSE;
			if (LE16(ctx->hdr->width) == 0 || 
			    LE16(ctx->hdr->height) == 0) {
				g_set_error_literal(err, GDK_PIXBUF_ERROR,
                                                    GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                                    _("TGA image has invalid dimensions"));
				return FALSE;
			}
			if ((ctx->hdr->flags & TGA_INTERLEAVE_MASK) != TGA_INTERLEAVE_NONE) {
				g_set_error_literal(err, GDK_PIXBUF_ERROR, 
                                                    GDK_PIXBUF_ERROR_UNKNOWN_TYPE,
                                                    _("TGA image type not supported"));
				return FALSE;
			}
			switch (ctx->hdr->type) {
			    case TGA_TYPE_PSEUDOCOLOR:
			    case TGA_TYPE_RLE_PSEUDOCOLOR:
				    if (ctx->hdr->bpp != 8) {
					    g_set_error_literal(err, GDK_PIXBUF_ERROR, 
                                                                GDK_PIXBUF_ERROR_UNKNOWN_TYPE,
                                                                _("TGA image type not supported"));
					    return FALSE;
				    }
				    break;
			    case TGA_TYPE_TRUECOLOR:
			    case TGA_TYPE_RLE_TRUECOLOR:
				    if (ctx->hdr->bpp != 24 &&
					ctx->hdr->bpp != 32) {
					    g_set_error_literal(err, GDK_PIXBUF_ERROR, 
                                                                GDK_PIXBUF_ERROR_UNKNOWN_TYPE,
                                                                _("TGA image type not supported"));
					    return FALSE;
				    }			      
				    break;
			    case TGA_TYPE_GRAYSCALE:
			    case TGA_TYPE_RLE_GRAYSCALE:
				    if (ctx->hdr->bpp != 8 &&
					ctx->hdr->bpp != 16) {
					    g_set_error_literal(err, GDK_PIXBUF_ERROR, 
                                                                GDK_PIXBUF_ERROR_UNKNOWN_TYPE,
                                                                _("TGA image type not supported"));
					    return FALSE;
				    }
				    break;
			    default:
				    g_set_error_literal(err, GDK_PIXBUF_ERROR, 
                                                        GDK_PIXBUF_ERROR_UNKNOWN_TYPE,
                                                        _("TGA image type not supported"));
				    return FALSE;    
			}
			if (!fill_in_context(ctx, err))
				return FALSE;
		} else {
			return TRUE;
		}
	}
	if (!ctx->skipped_info) {
		if (ctx->in->size >= ctx->hdr->infolen) {
			ctx->in = io_buffer_free_segment(ctx->in, ctx->hdr->infolen, err);
			if (!ctx->in)
				return FALSE;
			ctx->skipped_info = TRUE;
		} else {
			return TRUE;
		}
	}
	if (ctx->hdr->has_cmap && !ctx->cmap) {
		if (ctx->in->size >= ctx->cmap_size) {
			if (!try_colormap(ctx, err))
				return FALSE;
		} else {
			return TRUE;
		}
	}
	if (!ctx->prepared) {
		if (ctx->pfunc)
			(*ctx->pfunc) (ctx->pbuf, NULL, ctx->udata);
		ctx->prepared = TRUE;
	}
	/* We shouldn't get here anyway. */
	return TRUE;
}