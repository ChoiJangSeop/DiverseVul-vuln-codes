static int parse_one_option(const char *option)
{
	if (skip_prefix(option, "max-pack-size=", &option)) {
		unsigned long v;
		if (!git_parse_ulong(option, &v))
			return 0;
		if (v < 8192) {
			warning("max-pack-size is now in bytes, assuming --max-pack-size=%lum", v);
			v *= 1024 * 1024;
		} else if (v < 1024 * 1024) {
			warning("minimum max-pack-size is 1 MiB");
			v = 1024 * 1024;
		}
		max_packsize = v;
	} else if (skip_prefix(option, "big-file-threshold=", &option)) {
		unsigned long v;
		if (!git_parse_ulong(option, &v))
			return 0;
		big_file_threshold = v;
	} else if (skip_prefix(option, "depth=", &option)) {
		option_depth(option);
	} else if (skip_prefix(option, "active-branches=", &option)) {
		option_active_branches(option);
	} else if (skip_prefix(option, "export-pack-edges=", &option)) {
		option_export_pack_edges(option);
	} else if (!strcmp(option, "quiet")) {
		show_stats = 0;
	} else if (!strcmp(option, "stats")) {
		show_stats = 1;
	} else {
		return 0;
	}

	return 1;
}