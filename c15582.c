void PackLinuxElf64::unpack(OutputFile *fo)
{
    if (e_phoff != sizeof(Elf64_Ehdr)) {// Phdrs not contiguous with Ehdr
        throwCantUnpack("bad e_phoff");
    }
    unsigned const c_phnum = get_te16(&ehdri.e_phnum);
    upx_uint64_t old_data_off = 0;
    upx_uint64_t old_data_len = 0;
    upx_uint64_t old_dtinit = 0;
    unsigned is_asl = 0;  // is Android Shared Library

    unsigned szb_info = sizeof(b_info);
    {
        upx_uint64_t const e_entry = get_te64(&ehdri.e_entry);
        if (e_entry < 0x401180
        &&  get_te16(&ehdri.e_machine)==Elf64_Ehdr::EM_386) { /* old style, 8-byte b_info */
            szb_info = 2*sizeof(unsigned);
        }
    }

    fi->seek(overlay_offset - sizeof(l_info), SEEK_SET);
    fi->readx(&linfo, sizeof(linfo));
    lsize = get_te16(&linfo.l_lsize);
    if (UPX_MAGIC_LE32 != get_le32(&linfo.l_magic)) {
        throwCantUnpack("l_info corrupted");
    }
    p_info hbuf;  fi->readx(&hbuf, sizeof(hbuf));
    unsigned orig_file_size = get_te32(&hbuf.p_filesize);
    blocksize = get_te32(&hbuf.p_blocksize);
    if ((u32_t)file_size > orig_file_size || blocksize > orig_file_size
        || !mem_size_valid(1, blocksize, OVERHEAD))
        throwCantUnpack("p_info corrupted");

    ibuf.alloc(blocksize + OVERHEAD);
    b_info bhdr; memset(&bhdr, 0, sizeof(bhdr));
    fi->readx(&bhdr, szb_info);
    ph.u_len = get_te32(&bhdr.sz_unc);
    ph.c_len = get_te32(&bhdr.sz_cpr);
    if (ph.c_len > (unsigned)file_size || ph.c_len == 0 || ph.u_len == 0
    ||  ph.u_len > orig_file_size)
        throwCantUnpack("b_info corrupted");
    ph.filter_cto = bhdr.b_cto8;

    MemBuffer u(ph.u_len);
    Elf64_Ehdr *const ehdr = (Elf64_Ehdr *)&u[0];
    Elf64_Phdr const *phdr = 0;

    // Uncompress Ehdr and Phdrs.
    if (ibuf.getSize() < ph.c_len)
        throwCompressedDataViolation();
    fi->readx(ibuf, ph.c_len);
    decompress(ibuf, (upx_byte *)ehdr, false);
    if (ehdr->e_type   !=ehdri.e_type
    ||  ehdr->e_machine!=ehdri.e_machine
    ||  ehdr->e_version!=ehdri.e_version
        // less strict for EM_PPC64 to workaround earlier bug
    ||  !( ehdr->e_flags==ehdri.e_flags
        || Elf64_Ehdr::EM_PPC64 == get_te16(&ehdri.e_machine))
    ||  ehdr->e_ehsize !=ehdri.e_ehsize
        // check EI_MAG[0-3], EI_CLASS, EI_DATA, EI_VERSION
    ||  memcmp(ehdr->e_ident, ehdri.e_ident, Elf64_Ehdr::EI_OSABI)) {
        throwCantUnpack("ElfXX_Ehdr corrupted");
    }
    fi->seek(- (off_t) (szb_info + ph.c_len), SEEK_CUR);

    unsigned const u_phnum = get_te16(&ehdr->e_phnum);
    unsigned total_in = 0;
    unsigned total_out = 0;
    unsigned c_adler = upx_adler32(NULL, 0);
    unsigned u_adler = upx_adler32(NULL, 0);
#define MAX_ELF_HDR 1024
    if ((umin64(MAX_ELF_HDR, ph.u_len) - sizeof(Elf64_Ehdr))/sizeof(Elf64_Phdr) < u_phnum) {
        throwCantUnpack("bad compressed e_phnum");
    }
#undef MAX_ELF_HDR

    // Packed ET_EXE has no PT_DYNAMIC.
    // Packed ET_DYN has original PT_DYNAMIC for info needed by rtld.
    Elf64_Phdr const *const dynhdr = elf_find_ptype(Elf64_Phdr::PT_DYNAMIC, phdri, c_phnum);
    bool const is_shlib = !!dynhdr;
    if (is_shlib) {
        // Unpack and output the Ehdr and Phdrs for real.
        // This depends on position within input file fi.
        unpackExtent(ph.u_len, fo, total_in, total_out,
            c_adler, u_adler, false, szb_info);

        // The first PT_LOAD.  Part is not compressed (for benefit of rtld.)
        fi->seek(0, SEEK_SET);
        fi->readx(ibuf, get_te64(&dynhdr->p_offset) + get_te64(&dynhdr->p_filesz));
        overlay_offset -= sizeof(linfo);
        xct_off = overlay_offset;
        e_shoff = get_te64(&ehdri.e_shoff);
        ibuf.subref("bad .e_shoff %#lx for %#lx", e_shoff, sizeof(Elf64_Shdr) * e_shnum);
        if (e_shoff && e_shnum) { // --android-shlib
            shdri = (Elf64_Shdr /*const*/ *)ibuf.subref(
                "bad Shdr table", e_shoff, sizeof(Elf64_Shdr)*e_shnum);
            upx_uint64_t xct_off2 = get_te64(&shdri->sh_offset);
            if (e_shoff == xct_off2) {
                xct_off = e_shoff;
            }
            // un-Relocate dynsym (DT_SYMTAB) which is below xct_off
            dynseg = (Elf64_Dyn const *)ibuf.subref(
                "bad DYNAMIC", get_te64(&dynhdr->p_offset), get_te64(&dynhdr->p_filesz));
            dynstr = (char const *)elf_find_dynamic(Elf64_Dyn::DT_STRTAB);
            sec_dynsym = elf_find_section_type(Elf64_Shdr::SHT_DYNSYM);
            if (sec_dynsym) {
                upx_uint64_t const off_dynsym = get_te64(&sec_dynsym->sh_offset);
                upx_uint64_t const sz_dynsym  = get_te64(&sec_dynsym->sh_size);
                Elf64_Sym *const sym0 = (Elf64_Sym *)ibuf.subref(
                    "bad dynsym", off_dynsym, sz_dynsym);
                Elf64_Sym *sym = sym0;
                for (int j = sz_dynsym / sizeof(Elf64_Sym); --j>=0; ++sym) {
                    upx_uint64_t symval = get_te64(&sym->st_value);
                    unsigned symsec = get_te16(&sym->st_shndx);
                    if (Elf64_Sym::SHN_UNDEF != symsec
                    &&  Elf64_Sym::SHN_ABS   != symsec
                    &&  xct_off <= symval) {
                        set_te64(&sym->st_value, symval - asl_delta);
                    }
                    if (Elf64_Sym::SHN_ABS == symsec && xct_off <= symval) {
                        adjABS(sym, 0u - asl_delta);
                    }
                }
            }
        }
        if (fo) {
            fo->write(ibuf + ph.u_len, xct_off - ph.u_len);
        }
        // Search the Phdrs of compressed
        int n_ptload = 0;
        phdr = (Elf64_Phdr *) (void *) (1+ (Elf64_Ehdr *)(unsigned char *)ibuf);
        for (unsigned j=0; j < u_phnum; ++phdr, ++j) {
            if (PT_LOAD64==get_te32(&phdr->p_type) && 0!=n_ptload++) {
                old_data_off = get_te64(&phdr->p_offset);
                old_data_len = get_te64(&phdr->p_filesz);
                break;
            }
        }

        total_in  = xct_off;
        total_out = xct_off;
        ph.u_len = 0;
        // Position the input for next unpackExtent.
        fi->seek(sizeof(linfo) + overlay_offset + sizeof(hbuf) + szb_info + ph.c_len, SEEK_SET);

        // Decompress and unfilter the tail of first PT_LOAD.
        phdr = (Elf64_Phdr *) (void *) (1+ ehdr);
        for (unsigned j=0; j < u_phnum; ++phdr, ++j) {
            if (PT_LOAD64==get_te32(&phdr->p_type)) {
                ph.u_len = get_te64(&phdr->p_filesz) - xct_off;
                break;
            }
        }
        unpackExtent(ph.u_len, fo, total_in, total_out,
            c_adler, u_adler, false, szb_info);
    }
    else {  // main executable
        // Decompress each PT_LOAD.
        bool first_PF_X = true;
        phdr = (Elf64_Phdr *) (void *) (1+ ehdr);  // uncompressed
        for (unsigned j=0; j < u_phnum; ++phdr, ++j) {
            if (PT_LOAD64==get_te32(&phdr->p_type)) {
                unsigned const filesz = get_te64(&phdr->p_filesz);
                unsigned const offset = get_te64(&phdr->p_offset);
                if (fo)
                    fo->seek(offset, SEEK_SET);
                if (Elf64_Phdr::PF_X & get_te32(&phdr->p_flags)) {
                    unpackExtent(filesz, fo, total_in, total_out,
                        c_adler, u_adler, first_PF_X, szb_info);
                    first_PF_X = false;
                }
                else {
                    unpackExtent(filesz, fo, total_in, total_out,
                        c_adler, u_adler, false, szb_info);
                }
            }
        }
    }
    phdr = phdri;
    load_va = 0;
    for (unsigned j=0; j < c_phnum; ++j) {
        if (PT_LOAD64==get_te32(&phdr->p_type)) {
            load_va = get_te64(&phdr->p_vaddr);
            break;
        }
    }
    if (0x1000==get_te64(&phdri[0].p_filesz)  // detect C_BASE style
    &&  0==get_te64(&phdri[1].p_offset)
    &&  0==get_te64(&phdri[0].p_offset)
    &&     get_te64(&phdri[1].p_filesz) == get_te64(&phdri[1].p_memsz)) {
        fi->seek(up4(get_te64(&phdr[1].p_memsz)), SEEK_SET);  // past the loader
    }
    else if (is_shlib
    ||  ((unsigned)(get_te64(&ehdri.e_entry) - load_va) + up4(lsize) +
                ph.getPackHeaderSize() + sizeof(overlay_offset))
            < up4(file_size)) {
        // Loader is not at end; skip past it.
        funpad4(fi);  // MATCH01
        unsigned d_info[6]; fi->readx(d_info, sizeof(d_info));
        if (0==old_dtinit) {
            old_dtinit = get_te32(&d_info[2 + (0==d_info[0])]);
            is_asl = 1u& get_te32(&d_info[0 + (0==d_info[0])]);
        }
        fi->seek(lsize - sizeof(d_info), SEEK_CUR);
    }

    // The gaps between PT_LOAD and after last PT_LOAD
    phdr = (Elf64_Phdr *)&u[sizeof(*ehdr)];
    upx_uint64_t hi_offset(0);
    for (unsigned j = 0; j < u_phnum; ++j) {
        if (PT_LOAD64==phdr[j].p_type
        &&  hi_offset < phdr[j].p_offset)
            hi_offset = phdr[j].p_offset;
    }
    for (unsigned j = 0; j < u_phnum; ++j) {
        unsigned const size = find_LOAD_gap(phdr, j, u_phnum);
        if (size) {
            unsigned const where = get_te64(&phdr[j].p_offset) +
                                   get_te64(&phdr[j].p_filesz);
            if (fo)
                fo->seek(where, SEEK_SET);
            unpackExtent(size, fo, total_in, total_out,
                c_adler, u_adler, false, szb_info,
                (phdr[j].p_offset != hi_offset));
        }
    }

    // check for end-of-file
    fi->readx(&bhdr, szb_info);
    unsigned const sz_unc = ph.u_len = get_te32(&bhdr.sz_unc);

    if (sz_unc == 0) { // uncompressed size 0 -> EOF
        // note: magic is always stored le32
        unsigned const sz_cpr = get_le32(&bhdr.sz_cpr);
        if (sz_cpr != UPX_MAGIC_LE32)  // sz_cpr must be h->magic
            throwCompressedDataViolation();
    }
    else { // extra bytes after end?
        throwCompressedDataViolation();
    }

    if (is_shlib) {
        // DT_INIT must be restored.
        // If android_shlib, then the asl_delta relocations must be un-done.
        int n_ptload = 0;
        upx_uint64_t load_off = 0;
        phdr = (Elf64_Phdr *)&u[sizeof(*ehdr)];
        for (unsigned j= 0; j < u_phnum; ++j, ++phdr) {
            if (PT_LOAD64==get_te32(&phdr->p_type) && 0!=n_ptload++) {
                load_off = get_te64(&phdr->p_offset);
                load_va = get_te64(&phdr->p_vaddr);
                fi->seek(old_data_off, SEEK_SET);
                fi->readx(ibuf, old_data_len);
                total_in  += old_data_len;
                total_out += old_data_len;

                Elf64_Phdr const *udynhdr = (Elf64_Phdr *)&u[sizeof(*ehdr)];
                for (unsigned j3= 0; j3 < u_phnum; ++j3, ++udynhdr)
                if (Elf64_Phdr::PT_DYNAMIC==get_te32(&udynhdr->p_type)) {
                    upx_uint64_t dt_pltrelsz(0), dt_jmprel(0);
                    upx_uint64_t dt_relasz(0), dt_rela(0);
                    upx_uint64_t const dyn_len = get_te64(&udynhdr->p_filesz);
                    upx_uint64_t const dyn_off = get_te64(&udynhdr->p_offset);
                    if ((unsigned long)file_size < (dyn_len + dyn_off)) {
                        char msg[50]; snprintf(msg, sizeof(msg),
                                "bad PT_DYNAMIC .p_filesz %#lx", (long unsigned)dyn_len);
                        throwCantUnpack(msg);
                    }
                    if (dyn_off < load_off) {
                        continue;  // Oops.  Not really is_shlib ?  [built by 'rust' ?]
                    }
                    Elf64_Dyn *dyn = (Elf64_Dyn *)((unsigned char *)ibuf +
                        (dyn_off - load_off));
                    dynseg = dyn; invert_pt_dynamic(dynseg);
                    for (unsigned j2= 0; j2 < dyn_len; ++dyn, j2 += sizeof(*dyn)) {
                        upx_uint64_t const tag = get_te64(&dyn->d_tag);
                        upx_uint64_t       val = get_te64(&dyn->d_val);
                        if (is_asl) switch (tag) {
                        case Elf64_Dyn::DT_RELASZ:   { dt_relasz   = val; } break;
                        case Elf64_Dyn::DT_RELA:     { dt_rela     = val; } break;
                        case Elf64_Dyn::DT_PLTRELSZ: { dt_pltrelsz = val; } break;
                        case Elf64_Dyn::DT_JMPREL:   { dt_jmprel   = val; } break;

                        case Elf64_Dyn::DT_PLTGOT:
                        case Elf64_Dyn::DT_PREINIT_ARRAY:
                        case Elf64_Dyn::DT_INIT_ARRAY:
                        case Elf64_Dyn::DT_FINI_ARRAY:
                        case Elf64_Dyn::DT_FINI: {
                            set_te64(&dyn->d_val, val - asl_delta);
                        }; break;
                        } // end switch()
                        if (upx_dt_init == tag) {
                            if (Elf64_Dyn::DT_INIT == tag) {
                                set_te64(&dyn->d_val, old_dtinit);
                                if (!old_dtinit) { // compressor took the slot
                                    dyn->d_tag = Elf64_Dyn::DT_NULL;
                                    dyn->d_val = 0;
                                }
                            }
                            else if (Elf64_Dyn::DT_INIT_ARRAY    == tag
                            ||       Elf64_Dyn::DT_PREINIT_ARRAY == tag) {
                                if (val < load_va || (long unsigned)file_size < (long unsigned)val) {
                                    char msg[50]; snprintf(msg, sizeof(msg),
                                            "Bad Dynamic tag %#lx %#lx",
                                            (long unsigned)tag, (long unsigned)val);
                                    throwCantUnpack(msg);
                                }
                                set_te64(&ibuf[val - load_va], old_dtinit
                                    + (is_asl ? asl_delta : 0));  // counter-act unRel64
                            }
                        }
                        // Modified DT_*.d_val are re-written later from ibuf[]
                    }
                    if (is_asl) {
                        lowmem.alloc(xct_off);
                        fi->seek(0, SEEK_SET);
                        fi->read(lowmem, xct_off);  // contains relocation tables
                        if (dt_relasz && dt_rela) {
                            Elf64_Rela *const rela0 = (Elf64_Rela *)lowmem.subref(
                                "bad Rela offset", dt_rela, dt_relasz);
                            unRela64(dt_rela, rela0, dt_relasz, ibuf, load_va, old_dtinit, fo);
                        }
                        if (dt_pltrelsz && dt_jmprel) { // FIXME:  overlap w/ DT_REL ?
                            Elf64_Rela *const jmp0 = (Elf64_Rela *)lowmem.subref(
                                "bad Jmprel offset", dt_jmprel, dt_pltrelsz);
                            unRela64(dt_jmprel, jmp0, dt_pltrelsz, ibuf, load_va, old_dtinit, fo);
                        }
                        // Modified relocation tables are re-written by unRela64
                    }
                }
                if (fo) {
                    fo->seek(get_te64(&phdr->p_offset), SEEK_SET);
                    fo->rewrite(ibuf, old_data_len);
                }
            }
        }
    }

    // update header with totals
    ph.c_len = total_in;
    ph.u_len = total_out;

    // all bytes must be written
    if (total_out != orig_file_size)
        throwEOFException();

    // finally test the checksums
    if (ph.c_adler != c_adler || ph.u_adler != u_adler)
        throwChecksumError();
}