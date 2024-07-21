static RList *symbols(RBinFile *arch) {
	RList *shared = r_list_newf ((RListFree)r_list_free);
	if (!shared) {
		return NULL;
	}
	RList *cobjs = r_list_newf ((RListFree)free);
	if (!cobjs) {
		r_list_free (shared);
		return NULL;
	}
	interned_table = r_list_newf ((RListFree)free);
	if (!interned_table) {
		r_list_free (shared);
		r_list_free (cobjs);
		return NULL;
	}
	r_list_append (shared, cobjs);
	r_list_append (shared, interned_table);
	arch->o->bin_obj = shared;
	RList *sections = r_list_newf ((RListFree)free);
	if (!sections) {
		r_list_free (shared);
		arch->o->bin_obj = NULL;
		return NULL;
	}
	RList *symbols = r_list_newf ((RListFree)free);
	if (!symbols) {
		r_list_free (shared);
		arch->o->bin_obj = NULL;
		r_list_free (sections);
		return NULL;
	}
	RBuffer *buffer = arch->buf;
	r_buf_seek (buffer, code_start_offset, R_BUF_SET);
	pyc_get_sections_symbols (sections, symbols, cobjs, buffer, version.magic);
	sections_cache = sections;
	return symbols;
}