_equalConstraint(const Constraint *a, const Constraint *b)
{
	COMPARE_SCALAR_FIELD(contype);
	COMPARE_STRING_FIELD(conname);
	COMPARE_SCALAR_FIELD(deferrable);
	COMPARE_SCALAR_FIELD(initdeferred);
	COMPARE_LOCATION_FIELD(location);
	COMPARE_SCALAR_FIELD(is_no_inherit);
	COMPARE_NODE_FIELD(raw_expr);
	COMPARE_STRING_FIELD(cooked_expr);
	COMPARE_NODE_FIELD(keys);
	COMPARE_NODE_FIELD(exclusions);
	COMPARE_NODE_FIELD(options);
	COMPARE_STRING_FIELD(indexname);
	COMPARE_STRING_FIELD(indexspace);
	COMPARE_STRING_FIELD(access_method);
	COMPARE_NODE_FIELD(where_clause);
	COMPARE_NODE_FIELD(pktable);
	COMPARE_NODE_FIELD(fk_attrs);
	COMPARE_NODE_FIELD(pk_attrs);
	COMPARE_SCALAR_FIELD(fk_matchtype);
	COMPARE_SCALAR_FIELD(fk_upd_action);
	COMPARE_SCALAR_FIELD(fk_del_action);
	COMPARE_NODE_FIELD(old_conpfeqop);
	COMPARE_SCALAR_FIELD(skip_validation);
	COMPARE_SCALAR_FIELD(initially_valid);

	return true;
}