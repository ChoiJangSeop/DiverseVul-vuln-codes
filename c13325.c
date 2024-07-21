makeOperatorDependencies(HeapTuple tuple,
						 bool makeExtensionDep,
						 bool isUpdate)
{
	Form_pg_operator oper = (Form_pg_operator) GETSTRUCT(tuple);
	ObjectAddress myself,
				referenced;
	ObjectAddresses *addrs;

	ObjectAddressSet(myself, OperatorRelationId, oper->oid);

	/*
	 * If we are updating the operator, delete any existing entries, except
	 * for extension membership which should remain the same.
	 */
	if (isUpdate)
	{
		deleteDependencyRecordsFor(myself.classId, myself.objectId, true);
		deleteSharedDependencyRecordsFor(myself.classId, myself.objectId, 0);
	}

	addrs = new_object_addresses();

	/* Dependency on namespace */
	if (OidIsValid(oper->oprnamespace))
	{
		ObjectAddressSet(referenced, NamespaceRelationId, oper->oprnamespace);
		add_exact_object_address(&referenced, addrs);
	}

	/* Dependency on left type */
	if (OidIsValid(oper->oprleft))
	{
		ObjectAddressSet(referenced, TypeRelationId, oper->oprleft);
		add_exact_object_address(&referenced, addrs);
	}

	/* Dependency on right type */
	if (OidIsValid(oper->oprright))
	{
		ObjectAddressSet(referenced, TypeRelationId, oper->oprright);
		add_exact_object_address(&referenced, addrs);
	}

	/* Dependency on result type */
	if (OidIsValid(oper->oprresult))
	{
		ObjectAddressSet(referenced, TypeRelationId, oper->oprresult);
		add_exact_object_address(&referenced, addrs);
	}

	/*
	 * NOTE: we do not consider the operator to depend on the associated
	 * operators oprcom and oprnegate. We would not want to delete this
	 * operator if those go away, but only reset the link fields; which is not
	 * a function that the dependency code can presently handle.  (Something
	 * could perhaps be done with objectSubId though.)	For now, it's okay to
	 * let those links dangle if a referenced operator is removed.
	 */

	/* Dependency on implementation function */
	if (OidIsValid(oper->oprcode))
	{
		ObjectAddressSet(referenced, ProcedureRelationId, oper->oprcode);
		add_exact_object_address(&referenced, addrs);
	}

	/* Dependency on restriction selectivity function */
	if (OidIsValid(oper->oprrest))
	{
		ObjectAddressSet(referenced, ProcedureRelationId, oper->oprrest);
		add_exact_object_address(&referenced, addrs);
	}

	/* Dependency on join selectivity function */
	if (OidIsValid(oper->oprjoin))
	{
		ObjectAddressSet(referenced, ProcedureRelationId, oper->oprjoin);
		add_exact_object_address(&referenced, addrs);
	}

	record_object_address_dependencies(&myself, addrs, DEPENDENCY_NORMAL);
	free_object_addresses(addrs);

	/* Dependency on owner */
	recordDependencyOnOwner(OperatorRelationId, oper->oid,
							oper->oprowner);

	/* Dependency on extension */
	if (makeExtensionDep)
		recordDependencyOnCurrentExtension(&myself, true);

	return myself;
}