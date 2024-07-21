	switch (type) {
#ifdef ELFCORE
	case ET_CORE:
		phnum = elf_getu16(swap, elfhdr.e_phnum);
		if (phnum > ms->elf_phnum_max)
			return toomany(ms, "program", phnum);
		flags |= FLAGS_IS_CORE;
		if (dophn_core(ms, clazz, swap, fd,
		    (off_t)elf_getu(swap, elfhdr.e_phoff), phnum,
		    (size_t)elf_getu16(swap, elfhdr.e_phentsize),
		    fsize, &flags) == -1)
			return -1;
		break;
#endif
	case ET_EXEC:
	case ET_DYN:
		phnum = elf_getu16(swap, elfhdr.e_phnum);
		if (phnum > ms->elf_phnum_max)
			return toomany(ms, "program", phnum);
		shnum = elf_getu16(swap, elfhdr.e_shnum);
		if (shnum > ms->elf_shnum_max)
			return toomany(ms, "section", shnum);
		if (dophn_exec(ms, clazz, swap, fd,
		    (off_t)elf_getu(swap, elfhdr.e_phoff), phnum,
		    (size_t)elf_getu16(swap, elfhdr.e_phentsize),
		    fsize, &flags, shnum) == -1)
			return -1;
		/*FALLTHROUGH*/
	case ET_REL:
		shnum = elf_getu16(swap, elfhdr.e_shnum);
		if (shnum > ms->elf_shnum_max)
			return toomany(ms, "section", shnum);
		if (doshn(ms, clazz, swap, fd,
		    (off_t)elf_getu(swap, elfhdr.e_shoff), shnum,
		    (size_t)elf_getu16(swap, elfhdr.e_shentsize),
		    fsize, &flags, elf_getu16(swap, elfhdr.e_machine),
		    (int)elf_getu16(swap, elfhdr.e_shstrndx)) == -1)
			return -1;
		break;

	default:
		break;
	}