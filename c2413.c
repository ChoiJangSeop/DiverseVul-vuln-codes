ProcessInterrupts(void)
{
	/* OK to accept interrupt now? */
	if (InterruptHoldoffCount != 0 || CritSectionCount != 0)
		return;
	InterruptPending = false;
	if (ProcDiePending)
	{
		ProcDiePending = false;
		QueryCancelPending = false;		/* ProcDie trumps QueryCancel */
		ImmediateInterruptOK = false;	/* not idle anymore */
		DisableNotifyInterrupt();
		DisableCatchupInterrupt();
		/* As in quickdie, don't risk sending to client during auth */
		if (ClientAuthInProgress && whereToSendOutput == DestRemote)
			whereToSendOutput = DestNone;
		if (IsAutoVacuumWorkerProcess())
			ereport(FATAL,
					(errcode(ERRCODE_ADMIN_SHUTDOWN),
					 errmsg("terminating autovacuum process due to administrator command")));
		else if (RecoveryConflictPending && RecoveryConflictRetryable)
		{
			pgstat_report_recovery_conflict(RecoveryConflictReason);
			ereport(FATAL,
					(errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
			  errmsg("terminating connection due to conflict with recovery"),
					 errdetail_recovery_conflict()));
		}
		else if (RecoveryConflictPending)
		{
			/* Currently there is only one non-retryable recovery conflict */
			Assert(RecoveryConflictReason == PROCSIG_RECOVERY_CONFLICT_DATABASE);
			pgstat_report_recovery_conflict(RecoveryConflictReason);
			ereport(FATAL,
					(errcode(ERRCODE_DATABASE_DROPPED),
			  errmsg("terminating connection due to conflict with recovery"),
					 errdetail_recovery_conflict()));
		}
		else
			ereport(FATAL,
					(errcode(ERRCODE_ADMIN_SHUTDOWN),
			 errmsg("terminating connection due to administrator command")));
	}
	if (ClientConnectionLost)
	{
		QueryCancelPending = false;		/* lost connection trumps QueryCancel */
		ImmediateInterruptOK = false;	/* not idle anymore */
		DisableNotifyInterrupt();
		DisableCatchupInterrupt();
		/* don't send to client, we already know the connection to be dead. */
		whereToSendOutput = DestNone;
		ereport(FATAL,
				(errcode(ERRCODE_CONNECTION_FAILURE),
				 errmsg("connection to client lost")));
	}
	if (QueryCancelPending)
	{
		QueryCancelPending = false;
		if (ClientAuthInProgress)
		{
			ImmediateInterruptOK = false;		/* not idle anymore */
			DisableNotifyInterrupt();
			DisableCatchupInterrupt();
			/* As in quickdie, don't risk sending to client during auth */
			if (whereToSendOutput == DestRemote)
				whereToSendOutput = DestNone;
			ereport(ERROR,
					(errcode(ERRCODE_QUERY_CANCELED),
					 errmsg("canceling authentication due to timeout")));
		}

		/*
		 * If LOCK_TIMEOUT and STATEMENT_TIMEOUT indicators are both set, we
		 * prefer to report the former; but be sure to clear both.
		 */
		if (get_timeout_indicator(LOCK_TIMEOUT, true))
		{
			ImmediateInterruptOK = false;		/* not idle anymore */
			(void) get_timeout_indicator(STATEMENT_TIMEOUT, true);
			DisableNotifyInterrupt();
			DisableCatchupInterrupt();
			ereport(ERROR,
					(errcode(ERRCODE_LOCK_NOT_AVAILABLE),
					 errmsg("canceling statement due to lock timeout")));
		}
		if (get_timeout_indicator(STATEMENT_TIMEOUT, true))
		{
			ImmediateInterruptOK = false;		/* not idle anymore */
			DisableNotifyInterrupt();
			DisableCatchupInterrupt();
			ereport(ERROR,
					(errcode(ERRCODE_QUERY_CANCELED),
					 errmsg("canceling statement due to statement timeout")));
		}
		if (IsAutoVacuumWorkerProcess())
		{
			ImmediateInterruptOK = false;		/* not idle anymore */
			DisableNotifyInterrupt();
			DisableCatchupInterrupt();
			ereport(ERROR,
					(errcode(ERRCODE_QUERY_CANCELED),
					 errmsg("canceling autovacuum task")));
		}
		if (RecoveryConflictPending)
		{
			ImmediateInterruptOK = false;		/* not idle anymore */
			RecoveryConflictPending = false;
			DisableNotifyInterrupt();
			DisableCatchupInterrupt();
			pgstat_report_recovery_conflict(RecoveryConflictReason);
			if (DoingCommandRead)
				ereport(FATAL,
						(errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
						 errmsg("terminating connection due to conflict with recovery"),
						 errdetail_recovery_conflict(),
				 errhint("In a moment you should be able to reconnect to the"
						 " database and repeat your command.")));
			else
				ereport(ERROR,
						(errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
				 errmsg("canceling statement due to conflict with recovery"),
						 errdetail_recovery_conflict()));
		}

		/*
		 * If we are reading a command from the client, just ignore the cancel
		 * request --- sending an extra error message won't accomplish
		 * anything.  Otherwise, go ahead and throw the error.
		 */
		if (!DoingCommandRead)
		{
			ImmediateInterruptOK = false;		/* not idle anymore */
			DisableNotifyInterrupt();
			DisableCatchupInterrupt();
			ereport(ERROR,
					(errcode(ERRCODE_QUERY_CANCELED),
					 errmsg("canceling statement due to user request")));
		}
	}
	/* If we get here, do nothing (probably, QueryCancelPending was reset) */
}