static void parse_relocation_info(struct MACH0_(obj_t) *bin, RSkipList *relocs, ut32 offset, ut32 num) {
	if (!num || !offset || (st32)num < 0) {
		return;
	}

	ut64 total_size = num * sizeof (struct relocation_info);
	if (offset > bin->size) {
		return;
	}
	if (total_size > bin->size) {
		total_size = bin->size - offset;
		num = total_size /= sizeof (struct relocation_info);
	}
	struct relocation_info *info = calloc (num, sizeof (struct relocation_info));
	if (!info) {
		return;
	}

	if (r_buf_read_at (bin->b, offset, (ut8 *) info, total_size) < total_size) {
		free (info);
		return;
	}

	size_t i;
	for (i = 0; i < num; i++) {
		struct relocation_info a_info = info[i];
		ut32 sym_num = a_info.r_symbolnum;
		if (sym_num > bin->nsymtab) {
			continue;
		}

		ut32 stridx = bin->symtab[sym_num].n_strx;
		char *sym_name = get_name (bin, stridx, false);
		if (!sym_name) {
			continue;
		}

		struct reloc_t *reloc = R_NEW0 (struct reloc_t);
		if (!reloc) {
			free (info);
			free (sym_name);
			return;
		}

		reloc->addr = offset_to_vaddr (bin, a_info.r_address);
		reloc->offset = a_info.r_address;
		reloc->ord = sym_num;
		reloc->type = a_info.r_type; // enum RelocationInfoType
		reloc->external = a_info.r_extern;
		reloc->pc_relative = a_info.r_pcrel;
		reloc->size = a_info.r_length;
		r_str_ncpy (reloc->name, sym_name, sizeof (reloc->name) - 1);
		r_skiplist_insert (relocs, reloc);
		free (sym_name);
	}
	free (info);
}