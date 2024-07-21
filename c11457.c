static int init_strtab(ELFOBJ *bin) {
	r_return_val_if_fail (!bin->strtab, false);

	if (!bin->shdr) {
		return false;
	}

	Elf_(Half) shstrndx = bin->ehdr.e_shstrndx;
	if (shstrndx != SHN_UNDEF && !is_shidx_valid (bin, shstrndx)) {
		return false;
	}

	/* sh_size must be lower than UT32_MAX and not equal to zero, to avoid bugs on malloc() */
	if (bin->shdr[shstrndx].sh_size > UT32_MAX) {
		return false;
	}
	if (!bin->shdr[shstrndx].sh_size) {
		return false;
	}
	bin->shstrtab_section = bin->strtab_section = &bin->shdr[shstrndx];
	bin->shstrtab_size = bin->shstrtab_section->sh_size;
	if (bin->shstrtab_size > bin->size) {
		return false;
	}
	if (bin->shstrtab_section->sh_offset > bin->size) {
		return false;
	}
	if (bin->shstrtab_section->sh_offset + bin->shstrtab_section->sh_size > bin->size) {
		return false;
	}

	if (!(bin->shstrtab = calloc (1, bin->shstrtab_size + 1))) {
		r_sys_perror ("malloc");
		bin->shstrtab = NULL;
		return false;
	}
	int res = r_buf_read_at (bin->b, bin->shstrtab_section->sh_offset, (ut8*)bin->shstrtab,
		bin->shstrtab_section->sh_size);
	if (res < 1) {
		R_LOG_ERROR ("read (shstrtab) at 0x%" PFMT64x, (ut64) bin->shstrtab_section->sh_offset);
		R_FREE (bin->shstrtab);
		return false;
	}
	bin->shstrtab[bin->shstrtab_section->sh_size] = '\0';

	sdb_num_set (bin->kv, "elf_shstrtab.offset", bin->shstrtab_section->sh_offset, 0);
	sdb_num_set (bin->kv, "elf_shstrtab.size", bin->shstrtab_section->sh_size, 0);

	return true;
}