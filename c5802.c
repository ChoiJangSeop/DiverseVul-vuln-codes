static void initialise_write_state(fz_context *ctx, pdf_document *doc, const pdf_write_options *in_opts, pdf_write_state *opts)
{
	int num;
	int xref_len = pdf_xref_len(ctx, doc);

	opts->do_incremental = in_opts->do_incremental;
	opts->do_ascii = in_opts->do_ascii;
	opts->do_tight = !in_opts->do_pretty;
	opts->do_expand = in_opts->do_decompress;
	opts->do_compress = in_opts->do_compress;
	opts->do_compress_images = in_opts->do_compress_images;
	opts->do_compress_fonts = in_opts->do_compress_fonts;

	opts->do_garbage = in_opts->do_garbage;
	opts->do_linear = in_opts->do_linear;
	opts->do_clean = in_opts->do_clean;
	opts->start = 0;
	opts->main_xref_offset = INT_MIN;

	/* We deliberately make these arrays long enough to cope with
	* 1 to n access rather than 0..n-1, and add space for 2 new
	* extra entries that may be required for linearization. */
	opts->use_list = fz_malloc_array(ctx, xref_len + 3, sizeof(int));
	opts->ofs_list = fz_malloc_array(ctx, xref_len + 3, sizeof(int64_t));
	opts->gen_list = fz_calloc(ctx, xref_len + 3, sizeof(int));
	opts->renumber_map = fz_malloc_array(ctx, xref_len + 3, sizeof(int));
	opts->rev_renumber_map = fz_malloc_array(ctx, xref_len + 3, sizeof(int));
	opts->continue_on_error = in_opts->continue_on_error;
	opts->errors = in_opts->errors;

	for (num = 0; num < xref_len; num++)
	{
		opts->use_list[num] = 0;
		opts->ofs_list[num] = 0;
		opts->renumber_map[num] = num;
		opts->rev_renumber_map[num] = num;
	}
}