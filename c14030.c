gql_intro_eval(agooErr err, gqlDoc doc, gqlSel sel, gqlValue result, int depth) {
    struct _gqlField	field;
    struct _gqlCobj	obj;

    if (0 == strcmp("__type", sel->name)) {
	if (1 < depth) {
	    return agoo_err_set(err, AGOO_ERR_EVAL, "__type can only be called from a query root.");
	}
	obj.clas = &root_class;
	obj.ptr = NULL;
    } else if (0 == strcmp("__schema", sel->name)) {
	if (1 < depth) {
	    return agoo_err_set(err, AGOO_ERR_EVAL, "__scheme can only be called from a query root.");
	}
	obj.clas = &root_class;
	obj.ptr = NULL;
    } else {
	return agoo_err_set(err, AGOO_ERR_EVAL, "%s can only be called from the query root.", sel->name);
    }
    memset(&field, 0, sizeof(field));
    field.name = sel->name;
    field.type = sel->type;

    doc->funcs.resolve = gql_cobj_resolve;
    doc->funcs.type = gql_cobj_ref_type;

    return gql_cobj_resolve(err, doc, &obj, &field, sel, result, depth);
}