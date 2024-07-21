CheckIndexCompatible(Oid oldId,
					 RangeVar *heapRelation,
					 char *accessMethodName,
					 List *attributeList,
					 List *exclusionOpNames)
{
	bool		isconstraint;
	Oid		   *typeObjectId;
	Oid		   *collationObjectId;
	Oid		   *classObjectId;
	Oid			accessMethodId;
	Oid			relationId;
	HeapTuple	tuple;
	Form_pg_index indexForm;
	Form_pg_am	accessMethodForm;
	bool		amcanorder;
	int16	   *coloptions;
	IndexInfo  *indexInfo;
	int			numberOfAttributes;
	int			old_natts;
	bool		isnull;
	bool		ret = true;
	oidvector  *old_indclass;
	oidvector  *old_indcollation;
	Relation	irel;
	int			i;
	Datum		d;

	/* Caller should already have the relation locked in some way. */
	relationId = RangeVarGetRelid(heapRelation, NoLock, false);

	/*
	 * We can pretend isconstraint = false unconditionally.  It only serves to
	 * decide the text of an error message that should never happen for us.
	 */
	isconstraint = false;

	numberOfAttributes = list_length(attributeList);
	Assert(numberOfAttributes > 0);
	Assert(numberOfAttributes <= INDEX_MAX_KEYS);

	/* look up the access method */
	tuple = SearchSysCache1(AMNAME, PointerGetDatum(accessMethodName));
	if (!HeapTupleIsValid(tuple))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("access method \"%s\" does not exist",
						accessMethodName)));
	accessMethodId = HeapTupleGetOid(tuple);
	accessMethodForm = (Form_pg_am) GETSTRUCT(tuple);
	amcanorder = accessMethodForm->amcanorder;
	ReleaseSysCache(tuple);

	/*
	 * Compute the operator classes, collations, and exclusion operators for
	 * the new index, so we can test whether it's compatible with the existing
	 * one.  Note that ComputeIndexAttrs might fail here, but that's OK:
	 * DefineIndex would have called this function with the same arguments
	 * later on, and it would have failed then anyway.
	 */
	indexInfo = makeNode(IndexInfo);
	indexInfo->ii_Expressions = NIL;
	indexInfo->ii_ExpressionsState = NIL;
	indexInfo->ii_PredicateState = NIL;
	indexInfo->ii_ExclusionOps = NULL;
	indexInfo->ii_ExclusionProcs = NULL;
	indexInfo->ii_ExclusionStrats = NULL;
	typeObjectId = (Oid *) palloc(numberOfAttributes * sizeof(Oid));
	collationObjectId = (Oid *) palloc(numberOfAttributes * sizeof(Oid));
	classObjectId = (Oid *) palloc(numberOfAttributes * sizeof(Oid));
	coloptions = (int16 *) palloc(numberOfAttributes * sizeof(int16));
	ComputeIndexAttrs(indexInfo,
					  typeObjectId, collationObjectId, classObjectId,
					  coloptions, attributeList,
					  exclusionOpNames, relationId,
					  accessMethodName, accessMethodId,
					  amcanorder, isconstraint);


	/* Get the soon-obsolete pg_index tuple. */
	tuple = SearchSysCache1(INDEXRELID, ObjectIdGetDatum(oldId));
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "cache lookup failed for index %u", oldId);
	indexForm = (Form_pg_index) GETSTRUCT(tuple);

	/*
	 * We don't assess expressions or predicates; assume incompatibility.
	 * Also, if the index is invalid for any reason, treat it as incompatible.
	 */
	if (!(heap_attisnull(tuple, Anum_pg_index_indpred) &&
		  heap_attisnull(tuple, Anum_pg_index_indexprs) &&
		  IndexIsValid(indexForm)))
	{
		ReleaseSysCache(tuple);
		return false;
	}

	/* Any change in operator class or collation breaks compatibility. */
	old_natts = indexForm->indnatts;
	Assert(old_natts == numberOfAttributes);

	d = SysCacheGetAttr(INDEXRELID, tuple, Anum_pg_index_indcollation, &isnull);
	Assert(!isnull);
	old_indcollation = (oidvector *) DatumGetPointer(d);

	d = SysCacheGetAttr(INDEXRELID, tuple, Anum_pg_index_indclass, &isnull);
	Assert(!isnull);
	old_indclass = (oidvector *) DatumGetPointer(d);

	ret = (memcmp(old_indclass->values, classObjectId,
				  old_natts * sizeof(Oid)) == 0 &&
		   memcmp(old_indcollation->values, collationObjectId,
				  old_natts * sizeof(Oid)) == 0);

	ReleaseSysCache(tuple);

	if (!ret)
		return false;

	/* For polymorphic opcintype, column type changes break compatibility. */
	irel = index_open(oldId, AccessShareLock);	/* caller probably has a lock */
	for (i = 0; i < old_natts; i++)
	{
		if (IsPolymorphicType(get_opclass_input_type(classObjectId[i])) &&
			irel->rd_att->attrs[i]->atttypid != typeObjectId[i])
		{
			ret = false;
			break;
		}
	}

	/* Any change in exclusion operator selections breaks compatibility. */
	if (ret && indexInfo->ii_ExclusionOps != NULL)
	{
		Oid		   *old_operators,
				   *old_procs;
		uint16	   *old_strats;

		RelationGetExclusionInfo(irel, &old_operators, &old_procs, &old_strats);
		ret = memcmp(old_operators, indexInfo->ii_ExclusionOps,
					 old_natts * sizeof(Oid)) == 0;

		/* Require an exact input type match for polymorphic operators. */
		if (ret)
		{
			for (i = 0; i < old_natts && ret; i++)
			{
				Oid			left,
							right;

				op_input_types(indexInfo->ii_ExclusionOps[i], &left, &right);
				if ((IsPolymorphicType(left) || IsPolymorphicType(right)) &&
					irel->rd_att->attrs[i]->atttypid != typeObjectId[i])
				{
					ret = false;
					break;
				}
			}
		}
	}

	index_close(irel, NoLock);
	return ret;
}