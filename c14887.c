int fdt_find_regions(const void *fdt, char * const inc[], int inc_count,
		     char * const exc_prop[], int exc_prop_count,
		     struct fdt_region region[], int max_regions,
		     char *path, int path_len, int add_string_tab)
{
	int stack[FDT_MAX_DEPTH] = { 0 };
	char *end;
	int nextoffset = 0;
	uint32_t tag;
	int count = 0;
	int start = -1;
	int depth = -1;
	int want = 0;
	int base = fdt_off_dt_struct(fdt);

	end = path;
	*end = '\0';
	do {
		const struct fdt_property *prop;
		const char *name;
		const char *str;
		int include = 0;
		int stop_at = 0;
		int offset;
		int len;

		offset = nextoffset;
		tag = fdt_next_tag(fdt, offset, &nextoffset);
		stop_at = nextoffset;

		switch (tag) {
		case FDT_PROP:
			include = want >= 2;
			stop_at = offset;
			prop = fdt_get_property_by_offset(fdt, offset, NULL);
			str = fdt_string(fdt, fdt32_to_cpu(prop->nameoff));
			if (!str)
				return -FDT_ERR_BADSTRUCTURE;
			if (str_in_list(str, exc_prop, exc_prop_count))
				include = 0;
			break;

		case FDT_NOP:
			include = want >= 2;
			stop_at = offset;
			break;

		case FDT_BEGIN_NODE:
			depth++;
			if (depth == FDT_MAX_DEPTH)
				return -FDT_ERR_BADSTRUCTURE;
			name = fdt_get_name(fdt, offset, &len);
			if (end - path + 2 + len >= path_len)
				return -FDT_ERR_NOSPACE;
			if (end != path + 1)
				*end++ = '/';
			strcpy(end, name);
			end += len;
			stack[depth] = want;
			if (want == 1)
				stop_at = offset;
			if (str_in_list(path, inc, inc_count))
				want = 2;
			else if (want)
				want--;
			else
				stop_at = offset;
			include = want;
			break;

		case FDT_END_NODE:
			/* Depth must never go below -1 */
			if (depth < 0)
				return -FDT_ERR_BADSTRUCTURE;
			include = want;
			want = stack[depth--];
			while (end > path && *--end != '/')
				;
			*end = '\0';
			break;

		case FDT_END:
			include = 1;
			break;
		}

		if (include && start == -1) {
			/* Should we merge with previous? */
			if (count && count <= max_regions &&
			    offset == region[count - 1].offset +
					region[count - 1].size - base)
				start = region[--count].offset - base;
			else
				start = offset;
		}

		if (!include && start != -1) {
			if (count < max_regions) {
				region[count].offset = base + start;
				region[count].size = stop_at - start;
			}
			count++;
			start = -1;
		}
	} while (tag != FDT_END);

	if (nextoffset != fdt_size_dt_struct(fdt))
		return -FDT_ERR_BADLAYOUT;

	/* Add a region for the END tag and the string table */
	if (count < max_regions) {
		region[count].offset = base + start;
		region[count].size = nextoffset - start;
		if (add_string_tab)
			region[count].size += fdt_size_dt_strings(fdt);
	}
	count++;

	return count;
}