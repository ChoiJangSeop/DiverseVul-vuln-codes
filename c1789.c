recoveryStopsAfter(XLogRecord *record)
{
	uint8		record_info;
	TimestampTz recordXtime;

	record_info = record->xl_info & ~XLR_INFO_MASK;

	/*
	 * There can be many restore points that share the same name; we stop
	 * at the first one.
	 */
	if (recoveryTarget == RECOVERY_TARGET_NAME &&
		record->xl_rmid == RM_XLOG_ID && record_info == XLOG_RESTORE_POINT)
	{
		xl_restore_point *recordRestorePointData;

		recordRestorePointData = (xl_restore_point *) XLogRecGetData(record);

		if (strcmp(recordRestorePointData->rp_name, recoveryTargetName) == 0)
		{
			recoveryStopAfter = true;
			recoveryStopXid = InvalidTransactionId;
			(void) getRecordTimestamp(record, &recoveryStopTime);
			strncpy(recoveryStopName, recordRestorePointData->rp_name, MAXFNAMELEN);

			ereport(LOG,
					(errmsg("recovery stopping at restore point \"%s\", time %s",
							recoveryStopName,
							timestamptz_to_str(recoveryStopTime))));
			return true;
		}
	}

	if (record->xl_rmid == RM_XACT_ID &&
		(record_info == XLOG_XACT_COMMIT_COMPACT ||
		 record_info == XLOG_XACT_COMMIT ||
		 record_info == XLOG_XACT_ABORT))
	{
		/* Update the last applied transaction timestamp */
		if (getRecordTimestamp(record, &recordXtime))
			SetLatestXTime(recordXtime);

		/*
		 * There can be only one transaction end record with this exact
		 * transactionid
		 *
		 * when testing for an xid, we MUST test for equality only, since
		 * transactions are numbered in the order they start, not the order
		 * they complete. A higher numbered xid will complete before you about
		 * 50% of the time...
		 */
		if (recoveryTarget == RECOVERY_TARGET_XID && recoveryTargetInclusive &&
			record->xl_xid == recoveryTargetXid)
		{
			recoveryStopAfter = true;
			recoveryStopXid = record->xl_xid;
			recoveryStopTime = recordXtime;
			recoveryStopName[0] = '\0';

			if (record_info == XLOG_XACT_COMMIT_COMPACT || record_info == XLOG_XACT_COMMIT)
			{
				ereport(LOG,
						(errmsg("recovery stopping after commit of transaction %u, time %s",
								recoveryStopXid,
								timestamptz_to_str(recoveryStopTime))));
			}
			else if (record_info == XLOG_XACT_ABORT)
			{
				ereport(LOG,
						(errmsg("recovery stopping after abort of transaction %u, time %s",
								recoveryStopXid,
								timestamptz_to_str(recoveryStopTime))));
			}
			return true;
		}
	}

	/* Check if we should stop as soon as reaching consistency */
	if (recoveryTarget == RECOVERY_TARGET_IMMEDIATE && reachedConsistency)
	{
		ereport(LOG,
				(errmsg("recovery stopping after reaching consistency")));

		recoveryStopAfter = true;
		recoveryStopXid = InvalidTransactionId;
		recoveryStopTime = 0;
		recoveryStopName[0] = '\0';
		return true;
	}

	return false;
}