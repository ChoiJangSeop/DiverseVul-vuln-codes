gql_eval_sels(agooErr err, gqlDoc doc, gqlRef ref, gqlField field, gqlSel sels, gqlValue result, int depth) {
    gqlSel	sel;
    gqlField	sf = NULL;

    // TBD if depth over max then return an error

    for (sel = sels; NULL != sel; sel = sel->next) {
	if (NULL != field) {
	    if (NULL == sel->name) {
		sf = field;
	    } else {
		sf = gql_type_get_field(field->type, sel->name);
	    }
	} else {
	    sf = NULL;
	}
	if (NULL != sel->inline_frag) {
	    if (frag_include(doc, sel->inline_frag, ref)) {
		if (AGOO_ERR_OK != gql_eval_sels(err, doc, ref, sf, sel->inline_frag->sels, result, depth)) {
		    return err->code;
		}
	    }
	} else if (NULL != sel->frag) {
	    gqlFrag	frag;

	    for (frag = doc->frags; NULL != frag; frag = frag->next) {
		if (NULL != frag->name && 0 == strcmp(frag->name, sel->frag)) {
		    if (frag_include(doc, frag, ref)) {
			if (AGOO_ERR_OK != gql_eval_sels(err, doc, ref, sf, frag->sels, result, depth)) {
			    return err->code;
			}
		    }
		}
	    }
	} else {
	    if (AGOO_ERR_OK != doc->funcs.resolve(err, doc, ref, sf, sel, result, depth)) {
		return err->code;
	    }
	}
    }
    return AGOO_ERR_OK;
}