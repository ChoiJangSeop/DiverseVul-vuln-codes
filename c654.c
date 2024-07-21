gdk_pixbuf__gif_image_load_increment (gpointer data,
                                      const guchar *buf, guint size,
                                      GError **error)
{
	gint retval;
	GifContext *context = (GifContext *) data;

        context->error = error;
        
	if (context->amount_needed == 0) {
		/* we aren't looking for some bytes. */
		/* we can use buf now, but we don't want to keep it around at all.
		 * it will be gone by the end of the call. */
		context->buf = (guchar*) buf; /* very dubious const cast */
		context->ptr = 0;
		context->size = size;
	} else {
		/* we need some bytes */
		if (size < context->amount_needed) {
			context->amount_needed -= size;
			/* copy it over and return */
			memcpy (context->buf + context->size, buf, size);
			context->size += size;
			return TRUE;
		} else if (size == context->amount_needed) {
			memcpy (context->buf + context->size, buf, size);
			context->size += size;
		} else {
			context->buf = g_realloc (context->buf, context->size + size);
			memcpy (context->buf + context->size, buf, size);
			context->size += size;
		}
	}

	retval = gif_main_loop (context);

	if (retval == -2)
		return FALSE;
	if (retval == -1) {
		/* we didn't have enough memory */
		/* prepare for the next image_load_increment */
		if (context->buf == buf) {
			g_assert (context->size == size);
			context->buf = (guchar *)g_new (guchar, context->amount_needed + (context->size - context->ptr));
			memcpy (context->buf, buf + context->ptr, context->size - context->ptr);
		} else {
			/* copy the left overs to the begining of the buffer */
			/* and realloc the memory */
			memmove (context->buf, context->buf + context->ptr, context->size - context->ptr);
			context->buf = g_realloc (context->buf, context->amount_needed + (context->size - context->ptr));
		}
		context->size = context->size - context->ptr;
		context->ptr = 0;
	} else {
		/* we are prolly all done */
		if (context->buf == buf)
			context->buf = NULL;
	}
	return TRUE;
}