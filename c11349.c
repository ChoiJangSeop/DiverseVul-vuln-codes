RList *r_bin_ne_get_relocs(r_bin_ne_obj_t *bin) {
	RList *segments = bin->segments;
	if (!segments) {
		return NULL;
	}
	RList *entries = bin->entries;
	if (!entries) {
		return NULL;
	}
	RList *symbols = bin->symbols;
	if (!symbols) {
		return NULL;
	}

	ut16 *modref = calloc (bin->ne_header->ModRefs, sizeof (ut16));
	if (!modref) {
		return NULL;
	}
	r_buf_read_at (bin->buf, (ut64)bin->ne_header->ModRefTable + bin->header_offset, (ut8 *)modref, bin->ne_header->ModRefs * sizeof (ut16));

	RList *relocs = r_list_newf (free);
	if (!relocs) {
		free (modref);
		return NULL;
	}

	RListIter *it;
	RBinSection *seg;
	int index = -1;
	r_list_foreach (segments, it, seg) {
		index++;
		if (!(bin->segment_entries[index].flags & RELOCINFO)) {
			continue;
		}
		ut32 off = seg->paddr + seg->size;
		ut32 start = off;
		ut16 length = r_buf_read_le16_at (bin->buf, off);
		if (!length) {
			continue;
		}
		off += 2;
		// size_t buf_size = r_buf_size (bin->buf);
		while (off < start + length * sizeof (NE_image_reloc_item)) {
			// && off + sizeof (NE_image_reloc_item) < buf_size)
			NE_image_reloc_item rel = {0};
			if (r_buf_read_at (bin->buf, off, (ut8 *)&rel, sizeof (rel)) < 1) {
				return NULL;
			}
			RBinReloc *reloc = R_NEW0 (RBinReloc);
			if (!reloc) {
				return NULL;
			}
			reloc->paddr = seg->paddr + rel.offset;
			switch (rel.type) {
			case LOBYTE:
				reloc->type = R_BIN_RELOC_8;
				break;
			case SEL_16:
			case OFF_16:
				reloc->type = R_BIN_RELOC_16;
				break;
			case POI_32:
			case OFF_32:
				reloc->type = R_BIN_RELOC_32;
				break;
			case POI_48:
				reloc->type = R_BIN_RELOC_64;
				break;
			}
			
			ut32 offset;
			if (rel.flags & (IMPORTED_ORD | IMPORTED_NAME)) {
				RBinImport *imp = R_NEW0 (RBinImport);
				if (!imp) {
					free (reloc);
					break;
				}
				char *name;
#if NE_BUG
				if (rel.index > 0 && rel.index < bin->ne_header->ModRefs) {
					offset = modref[rel.index - 1] + bin->header_offset + bin->ne_header->ImportNameTable;
					name = __read_nonnull_str_at (bin->buf, offset);
				} else {
					name = r_str_newf ("UnknownModule%d_%x", rel.index, off); // ????
				}
#else
				if (rel.index > bin->ne_header->ModRefs) {
					name = r_str_newf ("UnknownModule%d_%x", rel.index, off); // ????
				} else {
					offset = modref[rel.index - 1] + bin->header_offset + bin->ne_header->ImportNameTable;
					name = __read_nonnull_str_at (bin->buf, offset);
				}
#endif
				if (rel.flags & IMPORTED_ORD) {
					imp->ordinal = rel.func_ord;
					imp->name = r_str_newf ("%s.%s", name, __func_name_from_ord(name, rel.func_ord));
				} else {
					offset = bin->header_offset + bin->ne_header->ImportNameTable + rel.name_off;
					char *func = __read_nonnull_str_at (bin->buf, offset);
					imp->name = r_str_newf ("%s.%s", name, func);
					free (func);
				}
				free (name);
				reloc->import = imp;
			} else if (rel.flags & OSFIXUP) {
				// TODO
			} else {
				if (strstr (seg->name, "FIXED")) {
					RBinSection *s = r_list_get_n (segments, rel.segnum - 1);
					if (s) {
						offset = s->paddr + rel.segoff;
					} else {
						offset = -1;
					}
				} else {
					RBinAddr *entry = r_list_get_n (entries, rel.entry_ordinal - 1);
					if (entry) {
						offset = entry->paddr;
					} else {
						offset = -1;
					}
				}
				reloc->addend = offset;
				RBinSymbol *sym = NULL;
				RListIter *sit;
				r_list_foreach (symbols, sit, sym) {
					if (sym->paddr == reloc->addend) {
						reloc->symbol = sym;
						break;
					}
				}
			}

			if (rel.flags & ADDITIVE) {
				reloc->additive = 1;
				r_list_append (relocs, reloc);
			} else {
				do {
#if NE_BUG
					if (reloc->paddr + 4 < r_buf_size (bin->buf)) {
						break;
					}
#endif
					r_list_append (relocs, reloc);
					offset = r_buf_read_le16_at (bin->buf, reloc->paddr);
					RBinReloc *tmp = reloc;
					reloc = R_NEW0 (RBinReloc);
					if (!reloc) {
						break;
					}
					*reloc = *tmp;
					reloc->paddr = seg->paddr + offset;
				} while (offset != 0xFFFF);
				free (reloc);
			}

			off += sizeof (NE_image_reloc_item);
		}
	}
	free (modref);
	return relocs;
}