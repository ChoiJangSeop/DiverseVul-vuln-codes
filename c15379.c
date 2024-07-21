void PackLinuxElf64::pack1(OutputFile *fo, Filter & /*ft*/)
{
    fi->seek(0, SEEK_SET);
    fi->readx(&ehdri, sizeof(ehdri));
    assert(e_phoff == sizeof(Elf64_Ehdr));  // checked by canPack()
    sz_phdrs = e_phnum * get_te16(&ehdri.e_phentsize);

    Elf64_Phdr *phdr = phdri;
    note_size = 0;
    for (unsigned j=0; j < e_phnum; ++phdr, ++j) {
        if (PT_NOTE64 == get_te32(&phdr->p_type)) {
            note_size += up4(get_te64(&phdr->p_filesz));
        }
    }
    if (note_size) {
        note_body = New(unsigned char, note_size);
        note_size = 0;
    }
    phdr = phdri;
    for (unsigned j=0; j < e_phnum; ++phdr, ++j) {
        unsigned const type = get_te32(&phdr->p_type);
        if (PT_NOTE64 == type) {
            unsigned const len = get_te64(&phdr->p_filesz);
            fi->seek(get_te64(&phdr->p_offset), SEEK_SET);
            fi->readx(&note_body[note_size], len);
            note_size += up4(len);
        }
        if (PT_LOAD64 == type) {
            unsigned x = get_te64(&phdr->p_align) >> lg2_page;
            while (x>>=1) {
                ++lg2_page;
            }
        }
        if (PT_GNU_STACK64 == type) {
            gnu_stack = phdr;
        }
    }
    page_size =  1u  <<lg2_page;
    page_mask = ~0ull<<lg2_page;

    progid = 0;  // getRandomId();  not useful, so do not clutter
    sz_elf_hdrs = sizeof(ehdri) + sz_phdrs;
    if (0!=xct_off) {  // shared library
        sz_elf_hdrs = xct_off;
        lowmem.alloc(xct_off + (!opt->o_unix.android_shlib
            ? 0
            : e_shnum * sizeof(Elf64_Shdr)));
        memcpy(lowmem, file_image, xct_off);  // android omits Shdr here
        fo->write(lowmem, xct_off);  // < SHF_EXECINSTR (typ: in .plt or .init)
        if (opt->o_unix.android_shlib) {
            // In order to pacify the runtime linker on Android "O" ("Oreo"),
            // we will splice-in a 4KiB page that contains an "extra" copy
            // of the Shdr, any PT_NOTE above xct_off, and shstrtab.
            // File order: Ehdr, Phdr[], section contents below xct_off,
            //    Shdr_copy[], PT_NOTEs.hi, shstrtab.
            xct_va  += asl_delta;
            //xct_off += asl_delta;  // not yet

            // Relocate PT_DYNAMIC (in 2nd PT_LOAD)
            Elf64_Dyn *dyn = const_cast<Elf64_Dyn *>(dynseg);
            for (; dyn->d_tag; ++dyn) {
                upx_uint64_t d_tag = get_te64(&dyn->d_tag);
                if (Elf64_Dyn::DT_FINI       == d_tag
                ||  Elf64_Dyn::DT_FINI_ARRAY == d_tag
                ||  Elf64_Dyn::DT_INIT_ARRAY == d_tag
                ||  Elf64_Dyn::DT_PREINIT_ARRAY == d_tag
                ||  Elf64_Dyn::DT_PLTGOT      == d_tag) {
                    upx_uint64_t d_val = get_te64(&dyn->d_val);
                    set_te64(&dyn->d_val, asl_delta + d_val);
                }
            }

            // Relocate dynsym (DT_SYMTAB) which is below xct_va
            upx_uint64_t const off_dynsym = get_te64(&sec_dynsym->sh_offset);
            upx_uint64_t const sz_dynsym  = get_te64(&sec_dynsym->sh_size);
            Elf64_Sym *dyntym = (Elf64_Sym *)lowmem.subref(
                "bad dynsym", off_dynsym, sz_dynsym);
            Elf64_Sym *sym = dyntym;
            for (int j = sz_dynsym / sizeof(Elf64_Sym); --j>=0; ++sym) {
                upx_uint64_t symval = get_te64(&sym->st_value);
                unsigned symsec = get_te16(&sym->st_shndx);
                if (Elf64_Sym::SHN_UNDEF != symsec
                &&  Elf64_Sym::SHN_ABS   != symsec
                &&  xct_off <= symval) {
                    set_te64(&sym->st_value, asl_delta + symval);
                }
                if (Elf64_Sym::SHN_ABS == symsec && xct_off <= symval) {
                    adjABS(sym, asl_delta);
                }
            }

            // Relocate Phdr virtual addresses, but not physical offsets and sizes
            unsigned char buf_notes[512]; memset(buf_notes, 0, sizeof(buf_notes));
            unsigned len_notes = 0;
            phdr = (Elf64_Phdr *)lowmem.subref(
                "bad e_phoff", e_phoff, e_phnum * sizeof(Elf64_Phdr));
            for (unsigned j = 0; j < e_phnum; ++j, ++phdr) {
                upx_uint64_t offset = get_te64(&phdr->p_offset);
                if (xct_off <= offset) { // above the extra page
                    if (PT_NOTE64 == get_te32(&phdr->p_type)) {
                        upx_uint64_t memsz = get_te64(&phdr->p_memsz);
                        if (sizeof(buf_notes) < (memsz + len_notes)) {
                            throwCantPack("PT_NOTES too big");
                        }
                        set_te64(&phdr->p_vaddr,
                            len_notes + (e_shnum * sizeof(Elf64_Shdr)) + xct_off);
                        phdr->p_offset = phdr->p_paddr = phdr->p_vaddr;
                        memcpy(&buf_notes[len_notes], &file_image[offset], memsz);
                        len_notes += memsz;
                    }
                    else {
                        //set_te64(&phdr->p_offset, asl_delta + offset);  // physical
                        upx_uint64_t addr = get_te64(&phdr->p_paddr);
                        set_te64(&phdr->p_paddr, asl_delta + addr);
                                     addr = get_te64(&phdr->p_vaddr);
                        set_te64(&phdr->p_vaddr, asl_delta + addr);
                    }
                }
                // .p_filesz,.p_memsz are updated in ::pack3
            }

            Elf64_Ehdr *const ehdr = (Elf64_Ehdr *)&lowmem[0];
            upx_uint64_t e_entry = get_te64(&ehdr->e_entry);
            if (xct_off < e_entry) {
                set_te64(&ehdr->e_entry, asl_delta + e_entry);
            }
            // Relocate Shdr; and Rela, Rel (below xct_off)
            set_te64(&ehdr->e_shoff, xct_off);
            memcpy(&lowmem[xct_off], shdri, e_shnum * sizeof(Elf64_Shdr));
            Elf64_Shdr *const shdro = (Elf64_Shdr *)&lowmem[xct_off];
            Elf64_Shdr *shdr = shdro;
            upx_uint64_t sz_shstrtab  = get_te64(&sec_strndx->sh_size);
            for (unsigned j = 0; j < e_shnum; ++j, ++shdr) {
                unsigned sh_type = get_te32(&shdr->sh_type);
                upx_uint64_t sh_size = get_te64(&shdr->sh_size);
                upx_uint64_t sh_offset = get_te64(&shdr->sh_offset);
                upx_uint64_t sh_entsize = get_te64(&shdr->sh_entsize);

                if (xct_off <= sh_offset) {
                    upx_uint64_t addr = get_te64(&shdr->sh_addr);
                    set_te64(&shdr->sh_addr, asl_delta + addr);
                }
                if (Elf64_Shdr::SHT_RELA == sh_type) {
                    if (sizeof(Elf64_Rela) != sh_entsize) {
                        char msg[50];
                        snprintf(msg, sizeof(msg), "bad Rela.sh_entsize %lu", (long)sh_entsize);
                        throwCantPack(msg);
                    }
                    n_jmp_slot = 0;
                    plt_off = ~0ull;
                    Elf64_Rela *const relb = (Elf64_Rela *)lowmem.subref(
                         "bad Rela offset", sh_offset, sh_size);
                    Elf64_Rela *rela = relb;
                    for (int k = sh_size / sh_entsize; --k >= 0; ++rela) {
                        upx_uint64_t r_addend = get_te64(&rela->r_addend);
                        upx_uint64_t r_offset = get_te64(&rela->r_offset);
                        upx_uint64_t r_info   = get_te64(&rela->r_info);
                        unsigned r_type = ELF64_R_TYPE(r_info);
                        if (xct_off <= r_offset) {
                            set_te64(&rela->r_offset, asl_delta + r_offset);
                        }
                        if (Elf64_Ehdr::EM_AARCH64 == e_machine) switch (r_type) {
                            default: {
                                char msg[90]; snprintf(msg, sizeof(msg),
                                    "unexpected relocation %#x [%#x]",
                                    r_type, -1 + (unsigned)(sh_size / sh_entsize) - k);
                                throwCantPack(msg);
                            } break;
                            case R_AARCH64_ABS64: // FALL THROUGH
                            case R_AARCH64_GLOB_DAT: // FALL THROUGH
                            case R_AARCH64_RELATIVE: {
                                if (xct_off <= r_addend) {
                                    set_te64(&rela->r_addend, asl_delta + r_addend);
                                }
                            } break;
                            case R_AARCH64_JUMP_SLOT: {
                                // .rela.plt contains offset of the "first time" target
                                if (plt_off > r_offset) {
                                    plt_off = r_offset;
                                }
                                upx_uint64_t d = elf_get_offset_from_address(r_offset);
                                upx_uint64_t w = get_te64(&file_image[d]);
                                if (xct_off <= w) {
                                    set_te64(&file_image[d], asl_delta + w);
                                }
                                ++n_jmp_slot;
                            } break;
                        }
                    }
                    fo->seek(sh_offset, SEEK_SET);
                    fo->rewrite(relb, sh_size);
                }
                if (Elf64_Shdr::SHT_REL == sh_type) {
                    if (sizeof(Elf64_Rel) != sh_entsize) {
                        char msg[50];
                        snprintf(msg, sizeof(msg), "bad Rel.sh_entsize %lu", (long)sh_entsize);
                        throwCantPack(msg);
                    }
                    Elf64_Rel *rel = (Elf64_Rel *)lowmem.subref(
                            "bad Rel sh_offset", sh_offset, sh_size);
                    for (int k = sh_size / sh_entsize; --k >= 0; ++rel) {
                        upx_uint64_t r_offset = get_te64(&rel->r_offset);
                        if (xct_off <= r_offset) {
                            set_te64(&rel->r_offset, asl_delta + r_offset);
                        }
                        // r_offset must be in 2nd PT_LOAD; .p_vaddr was already relocated
                        upx_uint64_t d = elf_get_offset_from_address(asl_delta + r_offset);
                        upx_uint64_t w = get_te64(&file_image[d]);
                        upx_uint64_t r_info = get_te64(&rel->r_info);
                        unsigned r_type = ELF64_R_TYPE(r_info);
                        if (xct_off <= w
                        &&  Elf64_Ehdr::EM_AARCH64 == e_machine
                        &&  (  R_AARCH64_RELATIVE  == r_type
                            || R_AARCH64_JUMP_SLOT == r_type)) {
                            set_te64(&file_image[d], asl_delta + w);
                        }
                    }
                }
                if (Elf64_Shdr::SHT_NOTE == sh_type) {
                    if (!(Elf64_Shdr::SHF_ALLOC & get_te64(&shdr->sh_flags))) {
                        // example: version numer of 'gold' linker (static binder)
                        if (sizeof(buf_notes) < (sh_size + len_notes)) {
                            throwCantPack("SHT_NOTEs too big");
                        }
                        set_te64(&shdro[j].sh_offset,
                            len_notes + (e_shnum * sizeof(Elf64_Shdr)) + xct_off);
                        memcpy(&buf_notes[len_notes], &file_image[sh_offset], sh_size);
                        len_notes += sh_size;
                    }
                    else { // SHF_ALLOC: in PT_LOAD; but move sh_addr and sh_offset
                        // Not sure why we need this conditional.
                        // Anyway, some Android have multiple SHT_NOTE sections.
                        if (xct_off <= sh_offset) {
                            upx_uint64_t pos = xct_off + e_shnum * sizeof(Elf64_Shdr);
                            set_te64(&shdr->sh_addr,   pos);
                            set_te64(&shdr->sh_offset, pos);
                        }
                    }
                }
            }
            // shstrndx will move
            set_te64(&shdro[get_te16(&ehdri.e_shstrndx)].sh_offset,
                len_notes + e_shnum * sizeof(Elf64_Shdr) + xct_off);

            // (Re-)write all changes below xct_off
            fo->seek(0, SEEK_SET);
            fo->rewrite(lowmem, xct_off);

            // New copy of Shdr
            Elf64_Shdr blank; memset(&blank, 0, sizeof(blank));
            set_te64(&blank.sh_offset, xct_off);  // hint for "upx -d"
            fo->write(&blank, sizeof(blank));
            fo->write(&shdro[1], (-1+ e_shnum) * sizeof(Elf64_Shdr));

            if (len_notes) {
                fo->write(buf_notes, len_notes);
            }

            // New copy of Shdr[.e_shstrndx].[ sh_offset, +.sh_size )
            fo->write(shstrtab,  sz_shstrtab);

            sz_elf_hdrs = fpad8(fo);
            //xct_off += asl_delta;  // wait until ::pack3
        }
        memset(&linfo, 0, sizeof(linfo));
        fo->write(&linfo, sizeof(linfo));
    }

    // only execute if option present
    if (opt->o_unix.preserve_build_id) {
        // set this so we can use elf_find_section_name
        e_shnum = get_te16(&ehdri.e_shnum);
        MemBuffer mb_shdri;
        if (!shdri) {
            mb_shdri.alloc(e_shnum * sizeof(Elf64_Shdr));
            shdri = (Elf64_Shdr *)mb_shdri.getVoidPtr();
            e_shoff = get_te64(&ehdri.e_shoff);
            fi->seek(e_shoff, SEEK_SET);
            fi->readx(shdri, e_shnum * sizeof(Elf64_Shdr));
        }
        //set the shstrtab
        sec_strndx = &shdri[get_te16(&ehdri.e_shstrndx)];

        upx_uint64_t sh_size = get_te64(&sec_strndx->sh_size);
        char *strtab = New(char, sh_size);
        fi->seek(0,SEEK_SET);
        fi->seek(sec_strndx->sh_offset,SEEK_SET);
        fi->readx(strtab, sh_size);

        shstrtab = (const char*)strtab;

        Elf64_Shdr const *buildid = elf_find_section_name(".note.gnu.build-id");
        if (buildid) {
            unsigned bid_sh_size = get_te32(&buildid->sh_size);
            unsigned char *data = New(unsigned char, bid_sh_size);
            memset(data,0,bid_sh_size);
            fi->seek(0,SEEK_SET);
            fi->seek(buildid->sh_offset,SEEK_SET);
            fi->readx(data,bid_sh_size);

            buildid_data  = data;

            o_elf_shnum = 3;
            memset(&shdrout,0,sizeof(shdrout));

            //setup the build-id
            memcpy(&shdrout.shdr[1], buildid, sizeof(shdrout.shdr[1]));
            set_te32(&shdrout.shdr[1].sh_name, 1);

            //setup the shstrtab
            memcpy(&shdrout.shdr[2], sec_strndx, sizeof(shdrout.shdr[2]));
            set_te32(&shdrout.shdr[2].sh_name, 20);
            set_te32(&shdrout.shdr[2].sh_size, 29); //size of our static shstrtab
        }
    }
}