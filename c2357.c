ExecBuildSlotValueDescription(TupleTableSlot *slot,
							  TupleDesc tupdesc,
							  int maxfieldlen)
{
	StringInfoData buf;
	bool		write_comma = false;
	int			i;

	/* Make sure the tuple is fully deconstructed */
	slot_getallattrs(slot);

	initStringInfo(&buf);

	appendStringInfoChar(&buf, '(');

	for (i = 0; i < tupdesc->natts; i++)
	{
		char	   *val;
		int			vallen;

		/* ignore dropped columns */
		if (tupdesc->attrs[i]->attisdropped)
			continue;

		if (slot->tts_isnull[i])
			val = "null";
		else
		{
			Oid			foutoid;
			bool		typisvarlena;

			getTypeOutputInfo(tupdesc->attrs[i]->atttypid,
							  &foutoid, &typisvarlena);
			val = OidOutputFunctionCall(foutoid, slot->tts_values[i]);
		}

		if (write_comma)
			appendStringInfoString(&buf, ", ");
		else
			write_comma = true;

		/* truncate if needed */
		vallen = strlen(val);
		if (vallen <= maxfieldlen)
			appendStringInfoString(&buf, val);
		else
		{
			vallen = pg_mbcliplen(val, vallen, maxfieldlen);
			appendBinaryStringInfo(&buf, val, vallen);
			appendStringInfoString(&buf, "...");
		}
	}

	appendStringInfoChar(&buf, ')');

	return buf.data;
}