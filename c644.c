gif_read (GifContext *context, guchar *buffer, size_t len)
{
	gint retval;
#ifdef IO_GIFDEBUG
	gint i;
#endif
	if (context->file) {
#ifdef IO_GIFDEBUG
		count += len;
		g_print ("Fsize :%d\tcount :%d\t", len, count);
#endif
		retval = (fread(buffer, len, 1, context->file) != 0);

                if (!retval && ferror (context->file))
                        g_set_error (context->error,
                                     G_FILE_ERROR,
                                     g_file_error_from_errno (errno),
                                     _("Failure reading GIF: %s"), strerror (errno));
                
#ifdef IO_GIFDEBUG
		if (len < 100) {
			for (i = 0; i < len; i++)
				g_print ("%d ", buffer[i]);
		}
		g_print ("\n");
#endif
                
		return retval;
	} else {
#ifdef IO_GIFDEBUG
/*  		g_print ("\tlooking for %d bytes.  size == %d, ptr == %d\n", len, context->size, context->ptr); */
#endif
		if ((context->size - context->ptr) >= len) {
#ifdef IO_GIFDEBUG
			count += len;
#endif
			memcpy (buffer, context->buf + context->ptr, len);
			context->ptr += len;
			context->amount_needed = 0;
#ifdef IO_GIFDEBUG
			g_print ("Psize :%d\tcount :%d\t", len, count);
			if (len < 100) {
				for (i = 0; i < len; i++)
					g_print ("%d ", buffer[i]);
			}
			g_print ("\n");
#endif
			return TRUE;
		}
		context->amount_needed = len - (context->size - context->ptr);
	}
	return 0;
}