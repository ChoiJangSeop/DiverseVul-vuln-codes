do_pdf_save_document(fz_context *ctx, pdf_document *doc, pdf_write_state *opts, pdf_write_options *in_opts)
{
	int lastfree;
	int num;
	int xref_len;

	if (in_opts->do_incremental)
	{
		/* If no changes, nothing to write */
		if (doc->num_incremental_sections == 0)
			return;
		if (opts->out)
		{
			fz_seek_output(ctx, opts->out, 0, SEEK_END);
			fz_write_string(ctx, opts->out, "\n");
		}
	}

	xref_len = pdf_xref_len(ctx, doc);

	fz_try(ctx)
	{
		initialise_write_state(ctx, doc, in_opts, opts);

		/* Make sure any objects hidden in compressed streams have been loaded */
		if (!opts->do_incremental)
		{
			pdf_ensure_solid_xref(ctx, doc, xref_len);
			preloadobjstms(ctx, doc);
		}

		/* Sweep & mark objects from the trailer */
		if (opts->do_garbage >= 1 || opts->do_linear)
			(void)markobj(ctx, doc, opts, pdf_trailer(ctx, doc));
		else
			for (num = 0; num < xref_len; num++)
				opts->use_list[num] = 1;

		/* Coalesce and renumber duplicate objects */
		if (opts->do_garbage >= 3)
			removeduplicateobjs(ctx, doc, opts);

		/* Compact xref by renumbering and removing unused objects */
		if (opts->do_garbage >= 2 || opts->do_linear)
			compactxref(ctx, doc, opts);

		/* Make renumbering affect all indirect references and update xref */
		if (opts->do_garbage >= 2 || opts->do_linear)
			renumberobjs(ctx, doc, opts);

		/* Truncate the xref after compacting and renumbering */
		if ((opts->do_garbage >= 2 || opts->do_linear) && !opts->do_incremental)
			while (xref_len > 0 && !opts->use_list[xref_len-1])
				xref_len--;

		if (opts->do_linear)
			linearize(ctx, doc, opts);

		if (opts->do_incremental)
		{
			int i;

			doc->disallow_new_increments = 1;

			for (i = 0; i < doc->num_incremental_sections; i++)
			{
				doc->xref_base = doc->num_incremental_sections - i - 1;

				writeobjects(ctx, doc, opts, 0);

#ifdef DEBUG_WRITING
				dump_object_details(ctx, doc, opts);
#endif

				for (num = 0; num < xref_len; num++)
				{
					if (!opts->use_list[num] && pdf_xref_is_incremental(ctx, doc, num))
					{
						/* Make unreusable. FIXME: would be better to link to existing free list */
						opts->gen_list[num] = 65535;
						opts->ofs_list[num] = 0;
					}
				}

				opts->first_xref_offset = fz_tell_output(ctx, opts->out);
				if (doc->has_xref_streams)
					writexrefstream(ctx, doc, opts, 0, xref_len, 1, 0, opts->first_xref_offset);
				else
					writexref(ctx, doc, opts, 0, xref_len, 1, 0, opts->first_xref_offset);

				doc->xref_sections[doc->xref_base].end_ofs = fz_tell_output(ctx, opts->out);
			}

			doc->xref_base = 0;
			doc->disallow_new_increments = 0;
		}
		else
		{
			writeobjects(ctx, doc, opts, 0);

#ifdef DEBUG_WRITING
			dump_object_details(ctx, doc, opts);
#endif

			/* Construct linked list of free object slots */
			lastfree = 0;
			for (num = 0; num < xref_len; num++)
			{
				if (!opts->use_list[num])
				{
					opts->gen_list[num]++;
					opts->ofs_list[lastfree] = num;
					lastfree = num;
				}
			}

			if (opts->do_linear && opts->page_count > 0)
			{
				opts->main_xref_offset = fz_tell_output(ctx, opts->out);
				writexref(ctx, doc, opts, 0, opts->start, 0, 0, opts->first_xref_offset);
				opts->file_len = fz_tell_output(ctx, opts->out);

				make_hint_stream(ctx, doc, opts);
				if (opts->do_ascii)
				{
					opts->hintstream_len *= 2;
					opts->hintstream_len += 1 + ((opts->hintstream_len+63)>>6);
				}
				opts->file_len += opts->hintstream_len;
				opts->main_xref_offset += opts->hintstream_len;
				update_linearization_params(ctx, doc, opts);
				fz_seek_output(ctx, opts->out, 0, 0);
				writeobjects(ctx, doc, opts, 1);

				padto(ctx, opts->out, opts->main_xref_offset);
				writexref(ctx, doc, opts, 0, opts->start, 0, 0, opts->first_xref_offset);
			}
			else
			{
				opts->first_xref_offset = fz_tell_output(ctx, opts->out);
				writexref(ctx, doc, opts, 0, xref_len, 1, 0, opts->first_xref_offset);
			}

			doc->xref_sections[0].end_ofs = fz_tell_output(ctx, opts->out);
		}

		doc->dirty = 0;
	}
	fz_always(ctx)
	{
#ifdef DEBUG_LINEARIZATION
		page_objects_dump(opts);
		objects_dump(ctx, doc, opts);
#endif
		finalise_write_state(ctx, opts);

		doc->freeze_updates = 0;
	}
	fz_catch(ctx)
	{
		fz_rethrow(ctx);
	}
}