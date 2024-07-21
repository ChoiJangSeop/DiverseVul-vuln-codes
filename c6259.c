PackLinuxElf64::canPack()
{
    union {
        unsigned char buf[sizeof(Elf64_Ehdr) + 14*sizeof(Elf64_Phdr)];
        //struct { Elf64_Ehdr ehdr; Elf64_Phdr phdr; } e;
    } u;
    COMPILE_TIME_ASSERT(sizeof(u) <= 1024)

    fi->readx(u.buf, sizeof(u.buf));
    fi->seek(0, SEEK_SET);
    Elf64_Ehdr const *const ehdr = (Elf64_Ehdr *) u.buf;

    // now check the ELF header
    if (checkEhdr(ehdr) != 0)
        return false;

    // additional requirements for linux/elf386
    if (get_te16(&ehdr->e_ehsize) != sizeof(*ehdr)) {
        throwCantPack("invalid Ehdr e_ehsize; try '--force-execve'");
        return false;
    }
    if (e_phoff != sizeof(*ehdr)) {// Phdrs not contiguous with Ehdr
        throwCantPack("non-contiguous Ehdr/Phdr; try '--force-execve'");
        return false;
    }

    // The first PT_LOAD64 must cover the beginning of the file (0==p_offset).
    Elf64_Phdr const *phdr = phdri;
    for (unsigned j=0; j < e_phnum; ++phdr, ++j) {
        if (j >= 14) {
            throwCantPack("too many ElfXX_Phdr; try '--force-execve'");
            return false;
        }
        unsigned const p_type = get_te32(&phdr->p_type);
        if (1!=exetype && PT_LOAD64 == p_type) { // 1st PT_LOAD
            exetype = 1;
            load_va = get_te64(&phdr->p_vaddr);  // class data member
            upx_uint64_t const p_offset = get_te64(&phdr->p_offset);
            upx_uint64_t const off = ~page_mask & load_va;
            if (off && off == p_offset) { // specific hint
                throwCantPack("Go-language PT_LOAD: try hemfix.c, or try '--force-execve'");
                // Fixing it inside upx fails because packExtent() reads original file.
                return false;
            }
            if (0 != p_offset) { // 1st PT_LOAD must cover Ehdr and Phdr
                throwCantPack("first PT_LOAD.p_offset != 0; try '--force-execve'");
                return false;
            }
            hatch_off = ~3ul & (3+ get_te64(&phdr->p_memsz));
            break;
        }
    }
    // We want to compress position-independent executable (gcc -pie)
    // main programs, but compressing a shared library must be avoided
    // because the result is no longer usable.  In theory, there is no way
    // to tell them apart: both are just ET_DYN.  Also in theory,
    // neither the presence nor the absence of any particular symbol name
    // can be used to tell them apart; there are counterexamples.
    // However, we will use the following heuristic suggested by
    // Peter S. Mazinger <ps.m@gmx.net> September 2005:
    // If a ET_DYN has __libc_start_main as a global undefined symbol,
    // then the file is a position-independent executable main program
    // (that depends on libc.so.6) and is eligible to be compressed.
    // Otherwise (no __libc_start_main as global undefined): skip it.
    // Also allow  __uClibc_main  and  __uClibc_start_main .

    if (Elf64_Ehdr::ET_DYN==get_te16(&ehdr->e_type)) {
        // The DT_SYMTAB has no designated length.  Read the whole file.
        alloc_file_image(file_image, file_size);
        fi->seek(0, SEEK_SET);
        fi->readx(file_image, file_size);
        memcpy(&ehdri, ehdr, sizeof(Elf64_Ehdr));
        phdri= (Elf64_Phdr *)((size_t)e_phoff + file_image);  // do not free() !!
        shdri= (Elf64_Shdr *)((size_t)e_shoff + file_image);  // do not free() !!

        sec_dynsym = elf_find_section_type(Elf64_Shdr::SHT_DYNSYM);
        if (sec_dynsym)
            sec_dynstr = get_te32(&sec_dynsym->sh_link) + shdri;

        sec_strndx = &shdri[get_te16(&ehdr->e_shstrndx)];
        shstrtab = (char const *)(get_te64(&sec_strndx->sh_offset) + file_image);
        if (Elf64_Shdr::SHT_STRTAB != get_te32(&sec_strndx->sh_type)
        || 0!=strcmp((char const *)".shstrtab",
                    &shstrtab[get_te32(&sec_strndx->sh_name)]) ) {
            throwCantPack("bad e_shstrndx");
        }

        phdr= phdri;
        for (int j= e_phnum; --j>=0; ++phdr)
        if (Elf64_Phdr::PT_DYNAMIC==get_te32(&phdr->p_type)) {
            dynseg= (Elf64_Dyn const *)(check_pt_dynamic(phdr) + file_image);
            invert_pt_dynamic(dynseg);
            break;
        }
        // elf_find_dynamic() returns 0 if 0==dynseg.
        dynstr=          (char const *)elf_find_dynamic(Elf64_Dyn::DT_STRTAB);
        dynsym=     (Elf64_Sym const *)elf_find_dynamic(Elf64_Dyn::DT_SYMTAB);

        if (opt->o_unix.force_pie
        ||       Elf64_Dyn::DF_1_PIE & elf_unsigned_dynamic(Elf64_Dyn::DT_FLAGS_1)
        ||  calls_crt1((Elf64_Rela const *)elf_find_dynamic(Elf64_Dyn::DT_RELA),
                                  (int)elf_unsigned_dynamic(Elf64_Dyn::DT_RELASZ))
        ||  calls_crt1((Elf64_Rela const *)elf_find_dynamic(Elf64_Dyn::DT_JMPREL),
                                  (int)elf_unsigned_dynamic(Elf64_Dyn::DT_PLTRELSZ))) {
            is_pie = true;
            goto proceed;  // calls C library init for main program
        }

        // Heuristic HACK for shared libraries (compare Darwin (MacOS) Dylib.)
        // If there is an existing DT_INIT, and if everything that the dynamic
        // linker ld-linux needs to perform relocations before calling DT_INIT
        // resides below the first SHT_EXECINSTR Section in one PT_LOAD, then
        // compress from the first executable Section to the end of that PT_LOAD.
        // We must not alter anything that ld-linux might touch before it calls
        // the DT_INIT function.
        //
        // Obviously this hack requires that the linker script put pieces
        // into good positions when building the original shared library,
        // and also requires ld-linux to behave.

        if (elf_find_dynamic(upx_dt_init)) {
            if (elf_has_dynamic(Elf64_Dyn::DT_TEXTREL)) {
                throwCantPack("DT_TEXTREL found; re-compile with -fPIC");
                goto abandon;
            }
            Elf64_Shdr const *shdr = shdri;
            xct_va = ~0ull;
            if (e_shnum) {
                for (int j= e_shnum; --j>=0; ++shdr) {
                    unsigned const sh_type = get_te64(&shdr->sh_type);
                    if (Elf64_Shdr::SHF_EXECINSTR & get_te64(&shdr->sh_flags)) {
                        xct_va = umin(xct_va, get_te64(&shdr->sh_addr));
                    }
                    // Hook the first slot of DT_PREINIT_ARRAY or DT_INIT_ARRAY.
                    if ((     Elf64_Dyn::DT_PREINIT_ARRAY==upx_dt_init
                        &&  Elf64_Shdr::SHT_PREINIT_ARRAY==sh_type)
                    ||  (     Elf64_Dyn::DT_INIT_ARRAY   ==upx_dt_init
                        &&  Elf64_Shdr::SHT_INIT_ARRAY   ==sh_type) ) {
                        user_init_off = get_te64(&shdr->sh_offset);
                        user_init_va = get_te64(&file_image[user_init_off]);
                    }
                    // By default /usr/bin/ld leaves 4 extra DT_NULL to support pre-linking.
                    // Take one as a last resort.
                    if ((Elf64_Dyn::DT_INIT==upx_dt_init || !upx_dt_init)
                    &&  Elf64_Shdr::SHT_DYNAMIC == sh_type) {
                        unsigned const n = get_te64(&shdr->sh_size) / sizeof(Elf64_Dyn);
                        Elf64_Dyn *dynp = (Elf64_Dyn *)&file_image[get_te64(&shdr->sh_offset)];
                        for (; Elf64_Dyn::DT_NULL != dynp->d_tag; ++dynp) {
                            if (upx_dt_init == get_te64(&dynp->d_tag)) {
                                break;  // re-found DT_INIT
                            }
                        }
                        if ((1+ dynp) < (n+ dynseg)) { // not the terminator, so take it
                            user_init_va = get_te64(&dynp->d_val);  // 0 if (0==upx_dt_init)
                            set_te64(&dynp->d_tag, upx_dt_init = Elf64_Dyn::DT_INIT);
                            user_init_off = (char const *)&dynp->d_val - (char const *)&file_image[0];
                        }
                    }
                }
            }
            else { // no Sections; use heuristics
                upx_uint64_t const strsz  = elf_unsigned_dynamic(Elf64_Dyn::DT_STRSZ);
                upx_uint64_t const strtab = elf_unsigned_dynamic(Elf64_Dyn::DT_STRTAB);
                upx_uint64_t const relsz  = elf_unsigned_dynamic(Elf64_Dyn::DT_RELSZ);
                upx_uint64_t const rel    = elf_unsigned_dynamic(Elf64_Dyn::DT_REL);
                upx_uint64_t const init   = elf_unsigned_dynamic(upx_dt_init);
                if ((init == (relsz + rel   ) && rel    == (strsz + strtab))
                ||  (init == (strsz + strtab) && strtab == (relsz + rel   ))
                ) {
                    xct_va = init;
                    user_init_va = init;
                    user_init_off = elf_get_offset_from_address(init);
                }
            }
            // Rely on 0==elf_unsigned_dynamic(tag) if no such tag.
            upx_uint64_t const va_gash = elf_unsigned_dynamic(Elf64_Dyn::DT_GNU_HASH);
            upx_uint64_t const va_hash = elf_unsigned_dynamic(Elf64_Dyn::DT_HASH);
            if (xct_va < va_gash  ||  (0==va_gash && xct_va < va_hash)
            ||  xct_va < elf_unsigned_dynamic(Elf64_Dyn::DT_STRTAB)
            ||  xct_va < elf_unsigned_dynamic(Elf64_Dyn::DT_SYMTAB)
            ||  xct_va < elf_unsigned_dynamic(Elf64_Dyn::DT_REL)
            ||  xct_va < elf_unsigned_dynamic(Elf64_Dyn::DT_RELA)
            ||  xct_va < elf_unsigned_dynamic(Elf64_Dyn::DT_JMPREL)
            ||  xct_va < elf_unsigned_dynamic(Elf64_Dyn::DT_VERDEF)
            ||  xct_va < elf_unsigned_dynamic(Elf64_Dyn::DT_VERSYM)
            ||  xct_va < elf_unsigned_dynamic(Elf64_Dyn::DT_VERNEEDED) ) {
                throwCantPack("DT_ tag above stub");
                goto abandon;
            }
            if (!opt->o_unix.android_shlib) {
                phdr = phdri;
                for (unsigned j= 0; j < e_phnum; ++phdr, ++j) {
                    upx_uint64_t const vaddr = get_te64(&phdr->p_vaddr);
                    if (PT_NOTE64 == get_te32(&phdr->p_type)
                    && xct_va < vaddr) {
                        char buf[40]; snprintf(buf, sizeof(buf),
                           "PT_NOTE %#lx above stub", (unsigned long)vaddr);
                        throwCantPack(buf);
                        goto abandon;
                    }
                }
            }
            xct_off = elf_get_offset_from_address(xct_va);
            if (opt->debug.debug_level) {
                fprintf(stderr, "shlib canPack: xct_va=%#lx  xct_off=%lx\n",
                    (long)xct_va, (long)xct_off);
            }
            goto proceed;  // But proper packing depends on checking xct_va.
        }
        else
            throwCantPack("need DT_INIT; try \"void _init(void){}\"");
abandon:
        return false;
proceed: ;
    }
    // XXX Theoretically the following test should be first,
    // but PackUnix::canPack() wants 0!=exetype ?
    if (!super::canPack())
        return false;
    assert(exetype == 1);
    exetype = 0;

    // set options
    opt->o_unix.blocksize = blocksize = file_size;
    return true;
}