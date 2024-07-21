PackLinuxElf32::check_pt_dynamic(Elf32_Phdr const *const phdr)
{
    unsigned t = get_te32(&phdr->p_offset), s = sizeof(Elf32_Dyn) + t;
    unsigned vaddr = get_te32(&phdr->p_vaddr);
    unsigned filesz = get_te32(&phdr->p_filesz), memsz = get_te32(&phdr->p_memsz);
    unsigned align = get_te32(&phdr->p_align);
    if (s < t || (u32_t)file_size < s
    ||  (3 & t) || (7 & (filesz | memsz))  // .balign 4; 8==sizeof(Elf32_Dyn)
    ||  (-1+ align) & (t ^ vaddr)
    ||  (unsigned long)file_size <= memsz
    ||  filesz < sizeof(Elf32_Dyn)
    ||  memsz  < sizeof(Elf32_Dyn)
    ||  filesz < memsz) {
        char msg[50]; snprintf(msg, sizeof(msg), "bad PT_DYNAMIC phdr[%u]",
            (unsigned)(phdr - phdri));
        throwCantPack(msg);
    }
    sz_dynseg = memsz;
    return t;
}