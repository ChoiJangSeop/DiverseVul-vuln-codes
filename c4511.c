do_file(char const *const fname)
{
	table_sort_t custom_sort;
	Elf32_Ehdr *ehdr = mmap_file(fname);

	ehdr_curr = ehdr;
	switch (ehdr->e_ident[EI_DATA]) {
	default:
		fprintf(stderr, "unrecognized ELF data encoding %d: %s\n",
			ehdr->e_ident[EI_DATA], fname);
		fail_file();
		break;
	case ELFDATA2LSB:
		r = rle;
		r2 = r2le;
		r8 = r8le;
		w = wle;
		w2 = w2le;
		w8 = w8le;
		break;
	case ELFDATA2MSB:
		r = rbe;
		r2 = r2be;
		r8 = r8be;
		w = wbe;
		w2 = w2be;
		w8 = w8be;
		break;
	}  /* end switch */
	if (memcmp(ELFMAG, ehdr->e_ident, SELFMAG) != 0
	||  r2(&ehdr->e_type) != ET_EXEC
	||  ehdr->e_ident[EI_VERSION] != EV_CURRENT) {
		fprintf(stderr, "unrecognized ET_EXEC file %s\n", fname);
		fail_file();
	}

	custom_sort = NULL;
	switch (r2(&ehdr->e_machine)) {
	default:
		fprintf(stderr, "unrecognized e_machine %d %s\n",
			r2(&ehdr->e_machine), fname);
		fail_file();
		break;
	case EM_386:
	case EM_X86_64:
	case EM_S390:
		custom_sort = sort_relative_table;
		break;
	case EM_ARCOMPACT:
	case EM_ARCV2:
	case EM_ARM:
	case EM_AARCH64:
	case EM_MICROBLAZE:
	case EM_MIPS:
	case EM_XTENSA:
		break;
	}  /* end switch */

	switch (ehdr->e_ident[EI_CLASS]) {
	default:
		fprintf(stderr, "unrecognized ELF class %d %s\n",
			ehdr->e_ident[EI_CLASS], fname);
		fail_file();
		break;
	case ELFCLASS32:
		if (r2(&ehdr->e_ehsize) != sizeof(Elf32_Ehdr)
		||  r2(&ehdr->e_shentsize) != sizeof(Elf32_Shdr)) {
			fprintf(stderr,
				"unrecognized ET_EXEC file: %s\n", fname);
			fail_file();
		}
		do32(ehdr, fname, custom_sort);
		break;
	case ELFCLASS64: {
		Elf64_Ehdr *const ghdr = (Elf64_Ehdr *)ehdr;
		if (r2(&ghdr->e_ehsize) != sizeof(Elf64_Ehdr)
		||  r2(&ghdr->e_shentsize) != sizeof(Elf64_Shdr)) {
			fprintf(stderr,
				"unrecognized ET_EXEC file: %s\n", fname);
			fail_file();
		}
		do64(ghdr, fname, custom_sort);
		break;
	}
	}  /* end switch */

	cleanup();
}