PackLinuxElf32::adjABS(Elf32_Sym *sym, unsigned delta)
{
    for (int j = 0; abs_symbol_names[j][0]; ++j) {
        unsigned st_name = get_te32(&sym->st_name);
        if (!strcmp(abs_symbol_names[j], get_str_name(st_name, (unsigned)-1))) {
            sym->st_value += delta;
            return 1;
        }
    }
    return 0;
}