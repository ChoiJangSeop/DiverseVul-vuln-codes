PackLinuxElf64::PackLinuxElf64help1(InputFile *f)
{
    e_type  = get_te16(&ehdri.e_type);
    e_phnum = get_te16(&ehdri.e_phnum);
    e_shnum = get_te16(&ehdri.e_shnum);
    unsigned const e_phentsize = get_te16(&ehdri.e_phentsize);
    if (ehdri.e_ident[Elf64_Ehdr::EI_CLASS]!=Elf64_Ehdr::ELFCLASS64
    || sizeof(Elf64_Phdr) != e_phentsize
    || (Elf64_Ehdr::ELFDATA2MSB == ehdri.e_ident[Elf64_Ehdr::EI_DATA]
            && &N_BELE_RTP::be_policy != bele)
    || (Elf64_Ehdr::ELFDATA2LSB == ehdri.e_ident[Elf64_Ehdr::EI_DATA]
            && &N_BELE_RTP::le_policy != bele)) {
        e_phoff = 0;
        e_shoff = 0;
        sz_phdrs = 0;
        return;
    }
    if (0==e_phnum) throwCantUnpack("0==e_phnum");
    e_phoff = get_te64(&ehdri.e_phoff);
    upx_uint64_t const last_Phdr = e_phoff + e_phnum * sizeof(Elf64_Phdr);
    if (last_Phdr < e_phoff || (unsigned long)file_size < last_Phdr) {
        throwCantUnpack("bad e_phoff");
    }
    e_shoff = get_te64(&ehdri.e_shoff);
    upx_uint64_t const last_Shdr = e_shoff + e_shnum * sizeof(Elf64_Shdr);
    if (last_Shdr < e_shoff || (unsigned long)file_size < last_Shdr) {
        if (opt->cmd == CMD_COMPRESS) {
            throwCantUnpack("bad e_shoff");
        }
    }
    sz_phdrs = e_phnum * e_phentsize;

    if (f && Elf64_Ehdr::ET_DYN!=e_type) {
        unsigned const len = sz_phdrs + e_phoff;
        alloc_file_image(file_image, len);
        f->seek(0, SEEK_SET);
        f->readx(file_image, len);
        phdri= (Elf64_Phdr       *)(e_phoff + file_image);  // do not free() !!
    }
    if (f && Elf64_Ehdr::ET_DYN==e_type) {
        // The DT_SYMTAB has no designated length.  Read the whole file.
        alloc_file_image(file_image, file_size);
        f->seek(0, SEEK_SET);
        f->readx(file_image, file_size);
        phdri= (Elf64_Phdr *)(e_phoff + file_image);  // do not free() !!
        shdri= (Elf64_Shdr *)(e_shoff + file_image);  // do not free() !!
        if (opt->cmd != CMD_COMPRESS) {
            shdri = NULL;
        }
        sec_dynsym = elf_find_section_type(Elf64_Shdr::SHT_DYNSYM);
        if (sec_dynsym) {
            unsigned t = get_te32(&sec_dynsym->sh_link);
            if (e_shnum <= t)
                throwCantPack("bad dynsym->sh_link");
            sec_dynstr = &shdri[t];
        }

        Elf64_Phdr const *phdr= phdri;
        for (int j = e_phnum; --j>=0; ++phdr)
        if (Elf64_Phdr::PT_DYNAMIC==get_te64(&phdr->p_type)) {
            dynseg= (Elf64_Dyn const *)(check_pt_dynamic(phdr) + file_image);
            invert_pt_dynamic(dynseg, get_te64(&phdr->p_filesz));
        }
        else if (PT_LOAD64==get_te32(&phdr->p_type)) {
            check_pt_load(phdr);
        }
        // elf_find_dynamic() returns 0 if 0==dynseg.
        dynstr =      (char const *)elf_find_dynamic(Elf64_Dyn::DT_STRTAB);
        dynsym = (Elf64_Sym const *)elf_find_dynamic(Elf64_Dyn::DT_SYMTAB);
        gashtab = (unsigned const *)elf_find_dynamic(Elf64_Dyn::DT_GNU_HASH);
        hashtab = (unsigned const *)elf_find_dynamic(Elf64_Dyn::DT_HASH);
        if (3& ((uintptr_t)dynsym | (uintptr_t)gashtab | (uintptr_t)hashtab)) {
            throwCantPack("unaligned DT_SYMTAB, DT_GNU_HASH, or DT_HASH/n");
        }
        jni_onload_sym = elf_lookup("JNI_OnLoad");
        if (jni_onload_sym) {
            jni_onload_va = get_te64(&jni_onload_sym->st_value);
            jni_onload_va = 0;  // FIXME not understood; need example
        }
    }
}