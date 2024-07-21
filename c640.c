lzw_read_byte (GifContext *context)
{
	int code, incode;
	gint retval;
	gint my_retval;
	register int i;

	if (context->lzw_code_pending != -1) {
		retval = context->lzw_code_pending;
		context->lzw_code_pending = -1;
		return retval;
	}

	if (context->lzw_fresh) {
		context->lzw_fresh = FALSE;
		do {
			retval = get_code (context, context->lzw_code_size);
			if (retval < 0) {
				return retval;
			}

			context->lzw_firstcode = context->lzw_oldcode = retval;
		} while (context->lzw_firstcode == context->lzw_clear_code);
		return context->lzw_firstcode;
	}

	if (context->lzw_sp > context->lzw_stack) {
		my_retval = *--(context->lzw_sp);
		return my_retval;
	}

	while ((code = get_code (context, context->lzw_code_size)) >= 0) {
		if (code == context->lzw_clear_code) {
			for (i = 0; i < context->lzw_clear_code; ++i) {
				context->lzw_table[0][i] = 0;
				context->lzw_table[1][i] = i;
			}
			for (; i < (1 << MAX_LZW_BITS); ++i)
				context->lzw_table[0][i] = context->lzw_table[1][i] = 0;
			context->lzw_code_size = context->lzw_set_code_size + 1;
			context->lzw_max_code_size = 2 * context->lzw_clear_code;
			context->lzw_max_code = context->lzw_clear_code + 2;
			context->lzw_sp = context->lzw_stack;

			set_gif_lzw_clear_code (context);
			return -3;
		} else if (code == context->lzw_end_code) {
			int count;
			unsigned char buf[260];

			/*g_error (" DID WE EVER EVER GET HERE\n");*/
			g_warning ("Unhandled Case.  If you have an image that causes this, let me <jrb@redhat.com> know.\n");

			if (ZeroDataBlock) {
				return -2;
			}

			while ((count = GetDataBlock (context, buf)) > 0)
				;

			if (count != 0) {
				/*g_print (_("GIF: missing EOD in data stream (common occurence)"));*/
				return -2;
			}
		}

		incode = code;

		if (code >= context->lzw_max_code) {
			*(context->lzw_sp)++ = context->lzw_firstcode;
			code = context->lzw_oldcode;
		}

		while (code >= context->lzw_clear_code) {
			*(context->lzw_sp)++ = context->lzw_table[1][code];

			if (code == context->lzw_table[0][code]) {
                                g_set_error (context->error,
                                             GDK_PIXBUF_ERROR,
                                             GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                             _("Circular table entry in GIF file"));
				return -2;
			}
			code = context->lzw_table[0][code];
		}

		*(context->lzw_sp)++ = context->lzw_firstcode = context->lzw_table[1][code];

		if ((code = context->lzw_max_code) < (1 << MAX_LZW_BITS)) {
			context->lzw_table[0][code] = context->lzw_oldcode;
			context->lzw_table[1][code] = context->lzw_firstcode;
			++context->lzw_max_code;
			if ((context->lzw_max_code >= context->lzw_max_code_size) &&
			    (context->lzw_max_code_size < (1 << MAX_LZW_BITS))) {
				context->lzw_max_code_size *= 2;
				++context->lzw_code_size;
			}
		}

		context->lzw_oldcode = incode;

		if (context->lzw_sp > context->lzw_stack) {
			my_retval = *--(context->lzw_sp);
			return my_retval;
		}
	}
	return code;
}