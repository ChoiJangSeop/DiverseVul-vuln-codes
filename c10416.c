static bool extract_sections_symbols(pyc_object *obj, RList *sections, RList *symbols, RList *cobjs, char *prefix) {
	pyc_code_object *cobj = NULL;
	RBinSection *section = NULL;
	RBinSymbol *symbol = NULL;
	RListIter *i = NULL;

	//each code object is a section
	if_true_return (!obj || (obj->type != TYPE_CODE_v1 && obj->type != TYPE_CODE_v0), false);

	cobj = obj->data;

	if_true_return (!cobj || !cobj->name, false);
	if_true_return (cobj->name->type != TYPE_ASCII && cobj->name->type != TYPE_STRING && cobj->name->type != TYPE_INTERNED, false);
	if_true_return (!cobj->name->data, false);
	if_true_return (!cobj->consts, false);

	//add the cobj to objs list
	if (!r_list_append (cobjs, cobj)) {
		goto fail;
	}
	section = R_NEW0 (RBinSection);
	symbol = R_NEW0 (RBinSymbol);
	prefix = r_str_newf ("%s%s%s", r_str_get (prefix),
		prefix? ".": "", (const char *)cobj->name->data);
	if (!prefix || !section || !symbol) {
		goto fail;
	}
	section->name = strdup (prefix);
	if (!section->name) {
		goto fail;
	}
	section->paddr = cobj->start_offset;
	section->vaddr = cobj->start_offset;
	section->size = cobj->end_offset - cobj->start_offset;
	section->vsize = cobj->end_offset - cobj->start_offset;
	if (!r_list_append (sections, section)) {
		goto fail;
	}
	// start building symbol
	symbol->name = strdup (prefix);
	//symbol->bind;
	symbol->type = R_BIN_TYPE_FUNC_STR;
	symbol->size = cobj->end_offset - cobj->start_offset;
	symbol->vaddr = cobj->start_offset;
	symbol->paddr = cobj->start_offset;
	symbol->ordinal = symbols_ordinal++;
	if (cobj->consts->type != TYPE_TUPLE && cobj->consts->type != TYPE_SMALL_TUPLE) {
		goto fail;
	}
	if (!r_list_append (symbols, symbol)) {
		goto fail;
	}
	r_list_foreach (((RList *)(cobj->consts->data)), i, obj) {
		extract_sections_symbols (obj, sections, symbols, cobjs, prefix);
	}
	free (prefix);
	return true;
fail:

	free (section);
	free (prefix);
	free (symbol);
	return false;
}