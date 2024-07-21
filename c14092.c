bool PackLinuxElf32::canPack()
{
    union {
        unsigned char buf[sizeof(Elf32_Ehdr) + 14*sizeof(Elf32_Phdr)];
        //struct { Elf32_Ehdr ehdr; Elf32_Phdr phdr; } e;
    } u;
    COMPILE_TIME_ASSERT(sizeof(u.buf) <= 512)

    fi->seek(0, SEEK_SET);
    fi->readx(u.buf, sizeof(u.buf));
    fi->seek(0, SEEK_SET);
    Elf32_Ehdr const *const ehdr = (Elf32_Ehdr *) u.buf;

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

    unsigned char osabi0 = u.buf[Elf32_Ehdr::EI_OSABI];
    // The first PT_LOAD32 must cover the beginning of the file (0==p_offset).
    Elf32_Phdr const *phdr = phdri;
    note_size = 0;
    for (unsigned j=0; j < e_phnum; ++phdr, ++j) {
        if (j >= 14) {
            throwCantPack("too many ElfXX_Phdr; try '--force-execve'");
            return false;
        }
        unsigned const p_type = get_te32(&phdr->p_type);
        unsigned const p_offset = get_te32(&phdr->p_offset);
        if (1!=exetype && PT_LOAD32 == p_type) { // 1st PT_LOAD
            exetype = 1;
            load_va = get_te32(&phdr->p_vaddr);  // class data member

            // Cast on next line is to avoid a compiler bug (incorrect complaint) in
            // Microsoft (R) C/C++ Optimizing Compiler Version 19.00.24215.1 for x64
            // error C4319: '~': zero extending 'unsigned int' to 'upx_uint64_t' of greater size
            unsigned const off = ~page_mask & (unsigned)load_va;

            if (off && off == p_offset) { // specific hint
                throwCantPack("Go-language PT_LOAD: try hemfix.c, or try '--force-execve'");
                // Fixing it inside upx fails because packExtent() reads original file.
                return false;
            }
            if (0 != p_offset) { // 1st PT_LOAD must cover Ehdr and Phdr
                throwCantPack("first PT_LOAD.p_offset != 0; try '--force-execve'");
                return false;
            }
            hatch_off = ~3u & (3+ get_te32(&phdr->p_memsz));
        }
        if (PT_NOTE32 == p_type) {
            unsigned const x = get_te32(&phdr->p_memsz);
            if ( sizeof(elfout.notes) < x  // beware overflow of note_size
            ||  (sizeof(elfout.notes) < (note_size += x)) ) {
                throwCantPack("PT_NOTEs too big; try '--force-execve'");
                return false;
            }
            if (osabi_note && Elf32_Ehdr::ELFOSABI_NONE==osabi0) { // Still seems to be generic.
                struct {
                    struct Elf32_Nhdr nhdr;
                    char name[8];
                    unsigned body;
                } note;
                memset(&note, 0, sizeof(note));
                fi->seek(p_offset, SEEK_SET);
                fi->readx(&note, sizeof(note));
                fi->seek(0, SEEK_SET);
                if (4==get_te32(&note.nhdr.descsz)
                &&  1==get_te32(&note.nhdr.type)
                // &&  0==note.end
                &&  (1+ strlen(osabi_note))==get_te32(&note.nhdr.namesz)
                &&  0==strcmp(osabi_note, (char const *)&note.name[0])
                ) {
                    osabi0 = ei_osabi;  // Specified by PT_NOTE.
                }
            }
        }
    }
    if (Elf32_Ehdr::ELFOSABI_NONE ==osabi0
    ||  Elf32_Ehdr::ELFOSABI_LINUX==osabi0) { // No EI_OSBAI, no PT_NOTE.
        unsigned const arm_eabi = 0xff000000u & get_te32(&ehdr->e_flags);
        if (Elf32_Ehdr::EM_ARM==e_machine
        &&   (EF_ARM_EABI_VER5==arm_eabi
          ||  EF_ARM_EABI_VER4==arm_eabi ) ) {
            // armel-eabi armeb-eabi ARM Linux EABI version 4 is a mess.
            ei_osabi = osabi0 = Elf32_Ehdr::ELFOSABI_LINUX;
        }
        else {
            osabi0 = opt->o_unix.osabi0;  // Possibly specified by command-line.
        }
    }
    if (osabi0!=ei_osabi) {
        return false;
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

    if (Elf32_Ehdr::ET_DYN==get_te16(&ehdr->e_type)) {
        // The DT_SYMTAB has no designated length.  Read the whole file.
        alloc_file_image(file_image, file_size);
        fi->seek(0, SEEK_SET);
        fi->readx(file_image, file_size);
        memcpy(&ehdri, ehdr, sizeof(Elf32_Ehdr));
        phdri= (Elf32_Phdr *)((size_t)e_phoff + file_image);  // do not free() !!
        shdri= (Elf32_Shdr *)((size_t)e_shoff + file_image);  // do not free() !!

        sec_strndx = NULL;
        shstrtab = NULL;
        if (e_shnum) {
            unsigned const e_shstrndx = get_te16(&ehdr->e_shstrndx);
            if (e_shstrndx) {
                if (e_shnum <= e_shstrndx) {
                    char msg[40]; snprintf(msg, sizeof(msg),
                        "bad e_shstrndx %#x >= e_shnum %d", e_shstrndx, e_shnum);
                    throwCantPack(msg);
                }
                sec_strndx = &shdri[e_shstrndx];
                unsigned const sh_offset = get_te32(&sec_strndx->sh_offset);
                if ((u32_t)file_size <= sh_offset) {
                    char msg[50]; snprintf(msg, sizeof(msg),
                        "bad .e_shstrndx->sh_offset %#x", sh_offset);
                    throwCantPack(msg);
                }
                shstrtab = (char const *)(sh_offset + file_image);
            }
            sec_dynsym = elf_find_section_type(Elf32_Shdr::SHT_DYNSYM);
            if (sec_dynsym) {
                unsigned const sh_link = get_te32(&sec_dynsym->sh_link);
                if (e_shnum <= sh_link) {
                    char msg[50]; snprintf(msg, sizeof(msg),
                        "bad SHT_DYNSYM.sh_link %#x", sh_link);
                }
                sec_dynstr = &shdri[sh_link];
            }

            if (sec_strndx) {
                unsigned const sh_name = get_te32(&sec_strndx->sh_name);
                if (Elf32_Shdr::SHT_STRTAB != get_te32(&sec_strndx->sh_type)
                || (u32_t)file_size <= sh_name  // FIXME: weak
                || (sh_name
                  && 0!=strcmp((char const *)".shstrtab", &shstrtab[sh_name]))
                ) {
                    throwCantPack("bad e_shstrndx");
                }
            }
        }

        Elf32_Phdr const *pload_x0(0);  // first eXecutable PT_LOAD
        phdr= phdri;
        for (int j= e_phnum; --j>=0; ++phdr)
        if (Elf32_Phdr::PT_DYNAMIC==get_te32(&phdr->p_type)) {
            dynseg= (Elf32_Dyn const *)(check_pt_dynamic(phdr) + file_image);
            invert_pt_dynamic(dynseg);
        }
        else if (PT_LOAD32==get_te32(&phdr->p_type)) {
            if (!pload_x0
            &&  Elf32_Phdr::PF_X & get_te32(&phdr->p_flags)
            ) {
                pload_x0 = phdr;
            }
            check_pt_load(phdr);
        }
        // elf_find_dynamic() returns 0 if 0==dynseg.
        dynstr=          (char const *)elf_find_dynamic(Elf32_Dyn::DT_STRTAB);
        dynsym=     (Elf32_Sym const *)elf_find_dynamic(Elf32_Dyn::DT_SYMTAB);

        if (opt->o_unix.force_pie
        ||      Elf32_Dyn::DF_1_PIE & elf_unsigned_dynamic(Elf32_Dyn::DT_FLAGS_1)
        ||  calls_crt1((Elf32_Rel const *)elf_find_dynamic(Elf32_Dyn::DT_REL),
                                 (int)elf_unsigned_dynamic(Elf32_Dyn::DT_RELSZ))
        ||  calls_crt1((Elf32_Rel const *)elf_find_dynamic(Elf32_Dyn::DT_JMPREL),
                                 (int)elf_unsigned_dynamic(Elf32_Dyn::DT_PLTRELSZ))) {
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

        // Apparently glibc-2.13.90 insists on 0==e_ident[EI_PAD..15],
        // so compressing shared libraries may be doomed anyway.
        // 2011-06-01: stub.shlib-init.S works around by installing hatch
        // at end of .text.

        if (/*jni_onload_sym ||*/ elf_find_dynamic(upx_dt_init)) {
            if (this->e_machine!=Elf32_Ehdr::EM_386
            &&  this->e_machine!=Elf32_Ehdr::EM_MIPS
            &&  this->e_machine!=Elf32_Ehdr::EM_ARM)
                goto abandon;  // need stub: EM_PPC
            if (elf_has_dynamic(Elf32_Dyn::DT_TEXTREL)) {
                throwCantPack("DT_TEXTREL found; re-compile with -fPIC");
                goto abandon;
            }
            if (!(Elf32_Dyn::DF_1_PIE & elf_unsigned_dynamic(Elf32_Dyn::DT_FLAGS_1))) {
                // not explicitly PIE main program
                if (Elf32_Ehdr::EM_ARM == e_machine  // Android is common
                &&  !opt->o_unix.android_shlib  // but not explicit
                ) {
                    opt->info_mode++;
                    info("note: use --android-shlib if appropriate");
                    opt->info_mode--;
                }
            }
            Elf32_Shdr const *shdr = shdri;
            xct_va = ~0u;
            if (e_shnum) {
                for (int j= e_shnum; --j>=0; ++shdr) {
                    unsigned const sh_type = get_te32(&shdr->sh_type);
                    if (Elf32_Shdr::SHF_EXECINSTR & get_te32(&shdr->sh_flags)) {
                        xct_va = umin(xct_va, get_te32(&shdr->sh_addr));
                    }
                    // Hook the first slot of DT_PREINIT_ARRAY or DT_INIT_ARRAY.
                    if ((     Elf32_Dyn::DT_PREINIT_ARRAY==upx_dt_init
                        &&  Elf32_Shdr::SHT_PREINIT_ARRAY==sh_type)
                    ||  (     Elf32_Dyn::DT_INIT_ARRAY   ==upx_dt_init
                        &&  Elf32_Shdr::SHT_INIT_ARRAY   ==sh_type) ) {
                        unsigned user_init_ava = get_te32(&shdr->sh_addr);
                        user_init_off = get_te32(&shdr->sh_offset);
                        if ((u32_t)file_size <= user_init_off) {
                            char msg[70]; snprintf(msg, sizeof(msg),
                                "bad Elf32_Shdr[%d].sh_offset %#x",
                                -1+ e_shnum - j, user_init_off);
                            throwCantPack(msg);
                        }
                        // Check that &file_image[user_init_off] has
                        // *_RELATIVE relocation, and fetch user_init_va.
                        // If Elf32_Rela then the actual value is in Rela.r_addend.
                        int z_rel = dt_table[Elf32_Dyn::DT_REL];
                        int z_rsz = dt_table[Elf32_Dyn::DT_RELSZ];
                        if (z_rel && z_rsz) {
                            unsigned rel_off = get_te32(&dynseg[-1+ z_rel].d_val);
                            Elf32_Rel *rp = (Elf32_Rel *)&file_image[rel_off];
                            unsigned relsz   = get_te32(&dynseg[-1+ z_rsz].d_val);
                            Elf32_Rel *last = (Elf32_Rel *)(relsz + (char *)rp);
                            for (; rp < last; ++rp) {
                                unsigned r_va = get_te32(&rp->r_offset);
                                if (r_va == user_init_ava) { // found the Elf32_Rel
                                    unsigned r_info = get_te32(&rp->r_info);
                                    unsigned r_type = ELF32_R_TYPE(r_info);
                                    if (Elf32_Ehdr::EM_ARM == e_machine
                                    &&  R_ARM_RELATIVE == r_type) {
                                        user_init_va = get_te32(&file_image[user_init_off]);
                                    }
                                    else {
                                        char msg[50]; snprintf(msg, sizeof(msg),
                                            "bad relocation %#x DT_INIT_ARRAY[0]",
                                            r_info);
                                        throwCantPack(msg);
                                    }
                                    break;
                                }
                            }
                        }
                        unsigned const p_filesz = get_te32(&pload_x0->p_filesz);
                        if (!((user_init_va - xct_va) < p_filesz)) {
                            // Not in executable portion of first executable PT_LOAD.
                            if (0==user_init_va && opt->o_unix.android_shlib) {
                                // Android allows (0 ==> skip) ?
                                upx_dt_init = 0;  // force steal of 'extra' DT_NULL
                                // XXX: FIXME: depends on SHT_DYNAMIC coming later
                            }
                            else {
                                char msg[70]; snprintf(msg, sizeof(msg),
                                    "bad init address %#x in Elf32_Shdr[%d].%#x\n",
                                    (unsigned)user_init_va, -1+ e_shnum - j, user_init_off);
                                throwCantPack(msg);
                            }
                        }
                    }
                    // By default /usr/bin/ld leaves 4 extra DT_NULL to support pre-linking.
                    // Take one as a last resort.
                    if ((Elf32_Dyn::DT_INIT==upx_dt_init || !upx_dt_init)
                    &&  Elf32_Shdr::SHT_DYNAMIC == sh_type) {
                        unsigned const n = get_te32(&shdr->sh_size) / sizeof(Elf32_Dyn);
                        Elf32_Dyn *dynp = (Elf32_Dyn *)&file_image[get_te32(&shdr->sh_offset)];
                        for (; Elf32_Dyn::DT_NULL != dynp->d_tag; ++dynp) {
                            if (upx_dt_init == get_te32(&dynp->d_tag)) {
                                break;  // re-found DT_INIT
                            }
                        }
                        if ((1+ dynp) < (n+ dynseg)) { // not the terminator, so take it
                            user_init_va = get_te32(&dynp->d_val);  // 0 if (0==upx_dt_init)
                            set_te32(&dynp->d_tag, upx_dt_init = Elf32_Dyn::DT_INIT);
                            user_init_off = (char const *)&dynp->d_val - (char const *)&file_image[0];
                        }
                    }
                }
            }
            else { // no Sections; use heuristics
                unsigned const strsz  = elf_unsigned_dynamic(Elf32_Dyn::DT_STRSZ);
                unsigned const strtab = elf_unsigned_dynamic(Elf32_Dyn::DT_STRTAB);
                unsigned const relsz  = elf_unsigned_dynamic(Elf32_Dyn::DT_RELSZ);
                unsigned const rel    = elf_unsigned_dynamic(Elf32_Dyn::DT_REL);
                unsigned const init   = elf_unsigned_dynamic(upx_dt_init);
                if ((init == (relsz + rel   ) && rel    == (strsz + strtab))
                ||  (init == (strsz + strtab) && strtab == (relsz + rel   ))
                ) {
                    xct_va = init;
                    user_init_va = init;
                    user_init_off = elf_get_offset_from_address(init);
                }
            }
            // Rely on 0==elf_unsigned_dynamic(tag) if no such tag.
            unsigned const va_gash = elf_unsigned_dynamic(Elf32_Dyn::DT_GNU_HASH);
            unsigned const va_hash = elf_unsigned_dynamic(Elf32_Dyn::DT_HASH);
            unsigned y = 0;
            if ((y=1, xct_va < va_gash)  ||  (y=2, (0==va_gash && xct_va < va_hash))
            ||  (y=3, xct_va < elf_unsigned_dynamic(Elf32_Dyn::DT_STRTAB))
            ||  (y=4, xct_va < elf_unsigned_dynamic(Elf32_Dyn::DT_SYMTAB))
            ||  (y=5, xct_va < elf_unsigned_dynamic(Elf32_Dyn::DT_REL))
            ||  (y=6, xct_va < elf_unsigned_dynamic(Elf32_Dyn::DT_RELA))
            ||  (y=7, xct_va < elf_unsigned_dynamic(Elf32_Dyn::DT_JMPREL))
            ||  (y=8, xct_va < elf_unsigned_dynamic(Elf32_Dyn::DT_VERDEF))
            ||  (y=9, xct_va < elf_unsigned_dynamic(Elf32_Dyn::DT_VERSYM))
            ||  (y=10, xct_va < elf_unsigned_dynamic(Elf32_Dyn::DT_VERNEEDED)) ) {
                static char const *which[] = {
                    "unknown",
                    "DT_GNU_HASH",
                    "DT_HASH",
                    "DT_STRTAB",
                    "DT_SYMTAB",
                    "DT_REL",
                    "DT_RELA",
                    "DT_JMPREL",
                    "DT_VERDEF",
                    "DT_VERSYM",
                    "DT_VERNEEDED",
                };
                char buf[30]; snprintf(buf, sizeof(buf), "%s above stub", which[y]);
                throwCantPack(buf);
                goto abandon;
            }
            if (!opt->o_unix.android_shlib) {
                phdr = phdri;
                for (unsigned j= 0; j < e_phnum; ++phdr, ++j) {
                    unsigned const vaddr = get_te32(&phdr->p_vaddr);
                    if (PT_NOTE32 == get_te32(&phdr->p_type)
                    && xct_va < vaddr) {
                        char buf[40]; snprintf(buf, sizeof(buf),
                           "PT_NOTE %#x above stub", vaddr);
                        throwCantPack(buf);
                        goto abandon;
                    }
                }
            }
            xct_off = elf_get_offset_from_address(xct_va);
            if (opt->debug.debug_level) {
                fprintf(stderr, "shlib canPack: xct_va=%#lx  xct_off=%#lx\n",
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