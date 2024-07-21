_copyConstraint(const Constraint *from)
{
	Constraint *newnode = makeNode(Constraint);

	COPY_SCALAR_FIELD(contype);
	COPY_STRING_FIELD(conname);
	COPY_SCALAR_FIELD(deferrable);
	COPY_SCALAR_FIELD(initdeferred);
	COPY_LOCATION_FIELD(location);
	COPY_SCALAR_FIELD(is_no_inherit);
	COPY_NODE_FIELD(raw_expr);
	COPY_STRING_FIELD(cooked_expr);
	COPY_NODE_FIELD(keys);
	COPY_NODE_FIELD(exclusions);
	COPY_NODE_FIELD(options);
	COPY_STRING_FIELD(indexname);
	COPY_STRING_FIELD(indexspace);
	COPY_STRING_FIELD(access_method);
	COPY_NODE_FIELD(where_clause);
	COPY_NODE_FIELD(pktable);
	COPY_NODE_FIELD(fk_attrs);
	COPY_NODE_FIELD(pk_attrs);
	COPY_SCALAR_FIELD(fk_matchtype);
	COPY_SCALAR_FIELD(fk_upd_action);
	COPY_SCALAR_FIELD(fk_del_action);
	COPY_NODE_FIELD(old_conpfeqop);
	COPY_SCALAR_FIELD(skip_validation);
	COPY_SCALAR_FIELD(initially_valid);

	return newnode;
}