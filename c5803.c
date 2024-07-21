static void removeduplicateobjs(fz_context *ctx, pdf_document *doc, pdf_write_state *opts)
{
	int num, other;
	int xref_len = pdf_xref_len(ctx, doc);

	for (num = 1; num < xref_len; num++)
	{
		/* Only compare an object to objects preceding it */
		for (other = 1; other < num; other++)
		{
			pdf_obj *a, *b;
			int newnum, streama = 0, streamb = 0, differ = 0;

			if (num == other || !opts->use_list[num] || !opts->use_list[other])
				continue;

			/* TODO: resolve indirect references to see if we can omit them */

			/*
			 * Comparing stream objects data contents would take too long.
			 *
			 * pdf_obj_num_is_stream calls pdf_cache_object and ensures
			 * that the xref table has the objects loaded.
			 */
			fz_try(ctx)
			{
				streama = pdf_obj_num_is_stream(ctx, doc, num);
				streamb = pdf_obj_num_is_stream(ctx, doc, other);
				differ = streama || streamb;
				if (streama && streamb && opts->do_garbage >= 4)
					differ = 0;
			}
			fz_catch(ctx)
			{
				/* Assume different */
				differ = 1;
			}
			if (differ)
				continue;

			a = pdf_get_xref_entry(ctx, doc, num)->obj;
			b = pdf_get_xref_entry(ctx, doc, other)->obj;

			if (pdf_objcmp(ctx, a, b))
				continue;

			if (streama && streamb)
			{
				/* Check to see if streams match too. */
				fz_buffer *sa = NULL;
				fz_buffer *sb = NULL;

				fz_var(sa);
				fz_var(sb);

				differ = 1;
				fz_try(ctx)
				{
					unsigned char *dataa, *datab;
					size_t lena, lenb;
					sa = pdf_load_raw_stream_number(ctx, doc, num);
					sb = pdf_load_raw_stream_number(ctx, doc, other);
					lena = fz_buffer_storage(ctx, sa, &dataa);
					lenb = fz_buffer_storage(ctx, sb, &datab);
					if (lena == lenb && memcmp(dataa, datab, lena) == 0)
						differ = 0;
				}
				fz_always(ctx)
				{
					fz_drop_buffer(ctx, sa);
					fz_drop_buffer(ctx, sb);
				}
				fz_catch(ctx)
				{
					fz_rethrow(ctx);
				}
				if (differ)
					continue;
			}

			/* Keep the lowest numbered object */
			newnum = fz_mini(num, other);
			opts->renumber_map[num] = newnum;
			opts->renumber_map[other] = newnum;
			opts->rev_renumber_map[newnum] = num; /* Either will do */
			opts->use_list[fz_maxi(num, other)] = 0;

			/* One duplicate was found, do not look for another */
			break;
		}
	}
}