CreateFKCheckTrigger(RangeVar *myRel, Constraint *fkconstraint,
					 Oid constraintOid, Oid indexOid, bool on_insert)
{
	CreateTrigStmt *fk_trigger;

	/*
	 * Note: for a self-referential FK (referencing and referenced tables are
	 * the same), it is important that the ON UPDATE action fires before the
	 * CHECK action, since both triggers will fire on the same row during an
	 * UPDATE event; otherwise the CHECK trigger will be checking a non-final
	 * state of the row.  Triggers fire in name order, so we ensure this by
	 * using names like "RI_ConstraintTrigger_a_NNNN" for the action triggers
	 * and "RI_ConstraintTrigger_c_NNNN" for the check triggers.
	 */
	fk_trigger = makeNode(CreateTrigStmt);
	fk_trigger->trigname = "RI_ConstraintTrigger_c";
	fk_trigger->relation = myRel;
	fk_trigger->row = true;
	fk_trigger->timing = TRIGGER_TYPE_AFTER;

	/* Either ON INSERT or ON UPDATE */
	if (on_insert)
	{
		fk_trigger->funcname = SystemFuncName("RI_FKey_check_ins");
		fk_trigger->events = TRIGGER_TYPE_INSERT;
	}
	else
	{
		fk_trigger->funcname = SystemFuncName("RI_FKey_check_upd");
		fk_trigger->events = TRIGGER_TYPE_UPDATE;
	}

	fk_trigger->columns = NIL;
	fk_trigger->whenClause = NULL;
	fk_trigger->isconstraint = true;
	fk_trigger->deferrable = fkconstraint->deferrable;
	fk_trigger->initdeferred = fkconstraint->initdeferred;
	fk_trigger->constrrel = fkconstraint->pktable;
	fk_trigger->args = NIL;

	(void) CreateTrigger(fk_trigger, NULL, constraintOid, indexOid, true);

	/* Make changes-so-far visible */
	CommandCounterIncrement();
}