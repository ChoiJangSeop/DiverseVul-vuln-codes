static RList *oneshotall_buffer(RBin *bin, RBuffer *b) {
	RList *list = r_list_newf (free);
	RBinXtrData *meta = get_the_meta (bin, b);
	r_list_append (list, meta);
	return list;
}