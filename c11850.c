static RzList *classes_from_symbols(RzBinFile *bf) {
	RzBinSymbol *sym;
	RzListIter *iter;
	rz_list_foreach (bf->o->symbols, iter, sym) {
		if (sym->name[0] != '_') {
			continue;
		}
		const char *cn = sym->classname;
		if (cn) {
			RzBinClass *c = rz_bin_file_add_class(bf, sym->classname, NULL, 0);
			if (!c) {
				continue;
			}
			// swift specific
			char *dn = sym->dname;
			char *fn = swiftField(dn, cn);
			if (fn) {
				RzBinField *f = rz_bin_field_new(sym->paddr, sym->vaddr, sym->size, fn, NULL, NULL, false);
				rz_list_append(c->fields, f);
				free(fn);
			} else {
				char *mn = strstr(dn, "..");
				if (!mn) {
					mn = strstr(dn, cn);
					if (mn && mn[strlen(cn)] == '.') {
						rz_list_append(c->methods, sym);
					}
				}
			}
		}
	}
	return bf->o->classes;
}