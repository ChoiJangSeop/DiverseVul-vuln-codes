ATPostAlterTypeCleanup(List **wqueue, AlteredTableInfo *tab, LOCKMODE lockmode)
{
	ObjectAddress obj;
	ListCell   *def_item;
	ListCell   *oid_item;

	/*
	 * Re-parse the index and constraint definitions, and attach them to the
	 * appropriate work queue entries.	We do this before dropping because in
	 * the case of a FOREIGN KEY constraint, we might not yet have exclusive
	 * lock on the table the constraint is attached to, and we need to get
	 * that before dropping.  It's safe because the parser won't actually look
	 * at the catalogs to detect the existing entry.
	 */
	forboth(oid_item, tab->changedConstraintOids,
			def_item, tab->changedConstraintDefs)
		ATPostAlterTypeParse(lfirst_oid(oid_item), (char *) lfirst(def_item),
							 wqueue, lockmode, tab->rewrite);
	forboth(oid_item, tab->changedIndexOids,
			def_item, tab->changedIndexDefs)
		ATPostAlterTypeParse(lfirst_oid(oid_item), (char *) lfirst(def_item),
							 wqueue, lockmode, tab->rewrite);

	/*
	 * Now we can drop the existing constraints and indexes --- constraints
	 * first, since some of them might depend on the indexes.  In fact, we
	 * have to delete FOREIGN KEY constraints before UNIQUE constraints, but
	 * we already ordered the constraint list to ensure that would happen. It
	 * should be okay to use DROP_RESTRICT here, since nothing else should be
	 * depending on these objects.
	 */
	foreach(oid_item, tab->changedConstraintOids)
	{
		obj.classId = ConstraintRelationId;
		obj.objectId = lfirst_oid(oid_item);
		obj.objectSubId = 0;
		performDeletion(&obj, DROP_RESTRICT, PERFORM_DELETION_INTERNAL);
	}

	foreach(oid_item, tab->changedIndexOids)
	{
		obj.classId = RelationRelationId;
		obj.objectId = lfirst_oid(oid_item);
		obj.objectSubId = 0;
		performDeletion(&obj, DROP_RESTRICT, PERFORM_DELETION_INTERNAL);
	}

	/*
	 * The objects will get recreated during subsequent passes over the work
	 * queue.
	 */
}