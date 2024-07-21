StartupXLOG(void)
{
	XLogCtlInsert *Insert;
	CheckPoint	checkPoint;
	bool		wasShutdown;
	bool		reachedStopPoint = false;
	bool		haveBackupLabel = false;
	XLogRecPtr	RecPtr,
				checkPointLoc,
				EndOfLog;
	XLogSegNo	endLogSegNo;
	TimeLineID	PrevTimeLineID;
	XLogRecord *record;
	TransactionId oldestActiveXID;
	bool		backupEndRequired = false;
	bool		backupFromStandby = false;
	DBState		dbstate_at_startup;
	XLogReaderState *xlogreader;
	XLogPageReadPrivate private;
	bool		fast_promoted = false;

	/*
	 * Read control file and check XLOG status looks valid.
	 *
	 * Note: in most control paths, *ControlFile is already valid and we need
	 * not do ReadControlFile() here, but might as well do it to be sure.
	 */
	ReadControlFile();

	if (ControlFile->state < DB_SHUTDOWNED ||
		ControlFile->state > DB_IN_PRODUCTION ||
		!XRecOffIsValid(ControlFile->checkPoint))
		ereport(FATAL,
				(errmsg("control file contains invalid data")));

	if (ControlFile->state == DB_SHUTDOWNED)
	{
		/* This is the expected case, so don't be chatty in standalone mode */
		ereport(IsPostmasterEnvironment ? LOG : NOTICE,
				(errmsg("database system was shut down at %s",
						str_time(ControlFile->time))));
	}
	else if (ControlFile->state == DB_SHUTDOWNED_IN_RECOVERY)
		ereport(LOG,
				(errmsg("database system was shut down in recovery at %s",
						str_time(ControlFile->time))));
	else if (ControlFile->state == DB_SHUTDOWNING)
		ereport(LOG,
				(errmsg("database system shutdown was interrupted; last known up at %s",
						str_time(ControlFile->time))));
	else if (ControlFile->state == DB_IN_CRASH_RECOVERY)
		ereport(LOG,
		   (errmsg("database system was interrupted while in recovery at %s",
				   str_time(ControlFile->time)),
			errhint("This probably means that some data is corrupted and"
					" you will have to use the last backup for recovery.")));
	else if (ControlFile->state == DB_IN_ARCHIVE_RECOVERY)
		ereport(LOG,
				(errmsg("database system was interrupted while in recovery at log time %s",
						str_time(ControlFile->checkPointCopy.time)),
				 errhint("If this has occurred more than once some data might be corrupted"
			  " and you might need to choose an earlier recovery target.")));
	else if (ControlFile->state == DB_IN_PRODUCTION)
		ereport(LOG,
			  (errmsg("database system was interrupted; last known up at %s",
					  str_time(ControlFile->time))));

	/* This is just to allow attaching to startup process with a debugger */
#ifdef XLOG_REPLAY_DELAY
	if (ControlFile->state != DB_SHUTDOWNED)
		pg_usleep(60000000L);
#endif

	/*
	 * Verify that pg_xlog and pg_xlog/archive_status exist.  In cases where
	 * someone has performed a copy for PITR, these directories may have been
	 * excluded and need to be re-created.
	 */
	ValidateXLOGDirectoryStructure();

	/*
	 * Clear out any old relcache cache files.	This is *necessary* if we do
	 * any WAL replay, since that would probably result in the cache files
	 * being out of sync with database reality.  In theory we could leave them
	 * in place if the database had been cleanly shut down, but it seems
	 * safest to just remove them always and let them be rebuilt during the
	 * first backend startup.
	 */
	RelationCacheInitFileRemove();

	/*
	 * Initialize on the assumption we want to recover to the latest timeline
	 * that's active according to pg_control.
	 */
	if (ControlFile->minRecoveryPointTLI >
		ControlFile->checkPointCopy.ThisTimeLineID)
		recoveryTargetTLI = ControlFile->minRecoveryPointTLI;
	else
		recoveryTargetTLI = ControlFile->checkPointCopy.ThisTimeLineID;

	/*
	 * Check for recovery control file, and if so set up state for offline
	 * recovery
	 */
	readRecoveryCommandFile();

	/*
	 * Save archive_cleanup_command in shared memory so that other processes
	 * can see it.
	 */
	strncpy(XLogCtl->archiveCleanupCommand,
			archiveCleanupCommand ? archiveCleanupCommand : "",
			sizeof(XLogCtl->archiveCleanupCommand));

	if (ArchiveRecoveryRequested)
	{
		if (StandbyModeRequested)
			ereport(LOG,
					(errmsg("entering standby mode")));
		else if (recoveryTarget == RECOVERY_TARGET_XID)
			ereport(LOG,
					(errmsg("starting point-in-time recovery to XID %u",
							recoveryTargetXid)));
		else if (recoveryTarget == RECOVERY_TARGET_TIME)
			ereport(LOG,
					(errmsg("starting point-in-time recovery to %s",
							timestamptz_to_str(recoveryTargetTime))));
		else if (recoveryTarget == RECOVERY_TARGET_NAME)
			ereport(LOG,
					(errmsg("starting point-in-time recovery to \"%s\"",
							recoveryTargetName)));
		else if (recoveryTarget == RECOVERY_TARGET_IMMEDIATE)
			ereport(LOG,
					(errmsg("starting point-in-time recovery to earliest consistent point")));
		else
			ereport(LOG,
					(errmsg("starting archive recovery")));
	}

	/*
	 * Take ownership of the wakeup latch if we're going to sleep during
	 * recovery.
	 */
	if (StandbyModeRequested)
		OwnLatch(&XLogCtl->recoveryWakeupLatch);

	/* Set up XLOG reader facility */
	MemSet(&private, 0, sizeof(XLogPageReadPrivate));
	xlogreader = XLogReaderAllocate(&XLogPageRead, &private);
	if (!xlogreader)
		ereport(ERROR,
				(errcode(ERRCODE_OUT_OF_MEMORY),
				 errmsg("out of memory"),
			errdetail("Failed while allocating an XLog reading processor.")));
	xlogreader->system_identifier = ControlFile->system_identifier;

	if (read_backup_label(&checkPointLoc, &backupEndRequired,
						  &backupFromStandby))
	{
		/*
		 * Archive recovery was requested, and thanks to the backup label
		 * file, we know how far we need to replay to reach consistency. Enter
		 * archive recovery directly.
		 */
		InArchiveRecovery = true;
		if (StandbyModeRequested)
			StandbyMode = true;

		/*
		 * When a backup_label file is present, we want to roll forward from
		 * the checkpoint it identifies, rather than using pg_control.
		 */
		record = ReadCheckpointRecord(xlogreader, checkPointLoc, 0, true);
		if (record != NULL)
		{
			memcpy(&checkPoint, XLogRecGetData(record), sizeof(CheckPoint));
			wasShutdown = (record->xl_info == XLOG_CHECKPOINT_SHUTDOWN);
			ereport(DEBUG1,
					(errmsg("checkpoint record is at %X/%X",
				   (uint32) (checkPointLoc >> 32), (uint32) checkPointLoc)));
			InRecovery = true;	/* force recovery even if SHUTDOWNED */

			/*
			 * Make sure that REDO location exists. This may not be the case
			 * if there was a crash during an online backup, which left a
			 * backup_label around that references a WAL segment that's
			 * already been archived.
			 */
			if (checkPoint.redo < checkPointLoc)
			{
				if (!ReadRecord(xlogreader, checkPoint.redo, LOG, false))
					ereport(FATAL,
							(errmsg("could not find redo location referenced by checkpoint record"),
							 errhint("If you are not restoring from a backup, try removing the file \"%s/backup_label\".", DataDir)));
			}
		}
		else
		{
			ereport(FATAL,
					(errmsg("could not locate required checkpoint record"),
					 errhint("If you are not restoring from a backup, try removing the file \"%s/backup_label\".", DataDir)));
			wasShutdown = false;	/* keep compiler quiet */
		}
		/* set flag to delete it later */
		haveBackupLabel = true;
	}
	else
	{
		/*
		 * It's possible that archive recovery was requested, but we don't
		 * know how far we need to replay the WAL before we reach consistency.
		 * This can happen for example if a base backup is taken from a
		 * running server using an atomic filesystem snapshot, without calling
		 * pg_start/stop_backup. Or if you just kill a running master server
		 * and put it into archive recovery by creating a recovery.conf file.
		 *
		 * Our strategy in that case is to perform crash recovery first,
		 * replaying all the WAL present in pg_xlog, and only enter archive
		 * recovery after that.
		 *
		 * But usually we already know how far we need to replay the WAL (up
		 * to minRecoveryPoint, up to backupEndPoint, or until we see an
		 * end-of-backup record), and we can enter archive recovery directly.
		 */
		if (ArchiveRecoveryRequested &&
			(ControlFile->minRecoveryPoint != InvalidXLogRecPtr ||
			 ControlFile->backupEndRequired ||
			 ControlFile->backupEndPoint != InvalidXLogRecPtr ||
			 ControlFile->state == DB_SHUTDOWNED))
		{
			InArchiveRecovery = true;
			if (StandbyModeRequested)
				StandbyMode = true;
		}

		/*
		 * Get the last valid checkpoint record.  If the latest one according
		 * to pg_control is broken, try the next-to-last one.
		 */
		checkPointLoc = ControlFile->checkPoint;
		RedoStartLSN = ControlFile->checkPointCopy.redo;
		record = ReadCheckpointRecord(xlogreader, checkPointLoc, 1, true);
		if (record != NULL)
		{
			ereport(DEBUG1,
					(errmsg("checkpoint record is at %X/%X",
				   (uint32) (checkPointLoc >> 32), (uint32) checkPointLoc)));
		}
		else if (StandbyMode)
		{
			/*
			 * The last valid checkpoint record required for a streaming
			 * recovery exists in neither standby nor the primary.
			 */
			ereport(PANIC,
					(errmsg("could not locate a valid checkpoint record")));
		}
		else
		{
			checkPointLoc = ControlFile->prevCheckPoint;
			record = ReadCheckpointRecord(xlogreader, checkPointLoc, 2, true);
			if (record != NULL)
			{
				ereport(LOG,
						(errmsg("using previous checkpoint record at %X/%X",
				   (uint32) (checkPointLoc >> 32), (uint32) checkPointLoc)));
				InRecovery = true;		/* force recovery even if SHUTDOWNED */
			}
			else
				ereport(PANIC,
					 (errmsg("could not locate a valid checkpoint record")));
		}
		memcpy(&checkPoint, XLogRecGetData(record), sizeof(CheckPoint));
		wasShutdown = (record->xl_info == XLOG_CHECKPOINT_SHUTDOWN);
	}

	/*
	 * If the location of the checkpoint record is not on the expected
	 * timeline in the history of the requested timeline, we cannot proceed:
	 * the backup is not part of the history of the requested timeline.
	 */
	Assert(expectedTLEs);		/* was initialized by reading checkpoint
								 * record */
	if (tliOfPointInHistory(checkPointLoc, expectedTLEs) !=
		checkPoint.ThisTimeLineID)
	{
		XLogRecPtr	switchpoint;

		/*
		 * tliSwitchPoint will throw an error if the checkpoint's timeline is
		 * not in expectedTLEs at all.
		 */
		switchpoint = tliSwitchPoint(ControlFile->checkPointCopy.ThisTimeLineID, expectedTLEs, NULL);
		ereport(FATAL,
				(errmsg("requested timeline %u is not a child of this server's history",
						recoveryTargetTLI),
				 errdetail("Latest checkpoint is at %X/%X on timeline %u, but in the history of the requested timeline, the server forked off from that timeline at %X/%X.",
						   (uint32) (ControlFile->checkPoint >> 32),
						   (uint32) ControlFile->checkPoint,
						   ControlFile->checkPointCopy.ThisTimeLineID,
						   (uint32) (switchpoint >> 32),
						   (uint32) switchpoint)));
	}

	/*
	 * The min recovery point should be part of the requested timeline's
	 * history, too.
	 */
	if (!XLogRecPtrIsInvalid(ControlFile->minRecoveryPoint) &&
	  tliOfPointInHistory(ControlFile->minRecoveryPoint - 1, expectedTLEs) !=
		ControlFile->minRecoveryPointTLI)
		ereport(FATAL,
				(errmsg("requested timeline %u does not contain minimum recovery point %X/%X on timeline %u",
						recoveryTargetTLI,
						(uint32) (ControlFile->minRecoveryPoint >> 32),
						(uint32) ControlFile->minRecoveryPoint,
						ControlFile->minRecoveryPointTLI)));

	LastRec = RecPtr = checkPointLoc;

	ereport(DEBUG1,
			(errmsg("redo record is at %X/%X; shutdown %s",
				  (uint32) (checkPoint.redo >> 32), (uint32) checkPoint.redo,
					wasShutdown ? "TRUE" : "FALSE")));
	ereport(DEBUG1,
			(errmsg("next transaction ID: %u/%u; next OID: %u",
					checkPoint.nextXidEpoch, checkPoint.nextXid,
					checkPoint.nextOid)));
	ereport(DEBUG1,
			(errmsg("next MultiXactId: %u; next MultiXactOffset: %u",
					checkPoint.nextMulti, checkPoint.nextMultiOffset)));
	ereport(DEBUG1,
			(errmsg("oldest unfrozen transaction ID: %u, in database %u",
					checkPoint.oldestXid, checkPoint.oldestXidDB)));
	ereport(DEBUG1,
			(errmsg("oldest MultiXactId: %u, in database %u",
					checkPoint.oldestMulti, checkPoint.oldestMultiDB)));
	if (!TransactionIdIsNormal(checkPoint.nextXid))
		ereport(PANIC,
				(errmsg("invalid next transaction ID")));

	/* initialize shared memory variables from the checkpoint record */
	ShmemVariableCache->nextXid = checkPoint.nextXid;
	ShmemVariableCache->nextOid = checkPoint.nextOid;
	ShmemVariableCache->oidCount = 0;
	MultiXactSetNextMXact(checkPoint.nextMulti, checkPoint.nextMultiOffset);
	SetTransactionIdLimit(checkPoint.oldestXid, checkPoint.oldestXidDB);
	SetMultiXactIdLimit(checkPoint.oldestMulti, checkPoint.oldestMultiDB);
	XLogCtl->ckptXidEpoch = checkPoint.nextXidEpoch;
	XLogCtl->ckptXid = checkPoint.nextXid;

	/*
	 * Initialize replication slots, before there's a chance to remove
	 * required resources.
	 */
	StartupReplicationSlots(checkPoint.redo);

	/*
	 * Startup MultiXact.  We need to do this early for two reasons: one
	 * is that we might try to access multixacts when we do tuple freezing,
	 * and the other is we need its state initialized because we attempt
	 * truncation during restartpoints.
	 */
	StartupMultiXact();

	/*
	 * Initialize unlogged LSN. On a clean shutdown, it's restored from the
	 * control file. On recovery, all unlogged relations are blown away, so
	 * the unlogged LSN counter can be reset too.
	 */
	if (ControlFile->state == DB_SHUTDOWNED)
		XLogCtl->unloggedLSN = ControlFile->unloggedLSN;
	else
		XLogCtl->unloggedLSN = 1;

	/*
	 * We must replay WAL entries using the same TimeLineID they were created
	 * under, so temporarily adopt the TLI indicated by the checkpoint (see
	 * also xlog_redo()).
	 */
	ThisTimeLineID = checkPoint.ThisTimeLineID;

	/*
	 * Copy any missing timeline history files between 'now' and the recovery
	 * target timeline from archive to pg_xlog. While we don't need those
	 * files ourselves - the history file of the recovery target timeline
	 * covers all the previous timelines in the history too - a cascading
	 * standby server might be interested in them. Or, if you archive the WAL
	 * from this server to a different archive than the master, it'd be good
	 * for all the history files to get archived there after failover, so that
	 * you can use one of the old timelines as a PITR target. Timeline history
	 * files are small, so it's better to copy them unnecessarily than not
	 * copy them and regret later.
	 */
	restoreTimeLineHistoryFiles(ThisTimeLineID, recoveryTargetTLI);

	lastFullPageWrites = checkPoint.fullPageWrites;

	RedoRecPtr = XLogCtl->RedoRecPtr = XLogCtl->Insert.RedoRecPtr = checkPoint.redo;

	if (RecPtr < checkPoint.redo)
		ereport(PANIC,
				(errmsg("invalid redo in checkpoint record")));

	/*
	 * Check whether we need to force recovery from WAL.  If it appears to
	 * have been a clean shutdown and we did not have a recovery.conf file,
	 * then assume no recovery needed.
	 */
	if (checkPoint.redo < RecPtr)
	{
		if (wasShutdown)
			ereport(PANIC,
					(errmsg("invalid redo record in shutdown checkpoint")));
		InRecovery = true;
	}
	else if (ControlFile->state != DB_SHUTDOWNED)
		InRecovery = true;
	else if (ArchiveRecoveryRequested)
	{
		/* force recovery due to presence of recovery.conf */
		InRecovery = true;
	}

	/* REDO */
	if (InRecovery)
	{
		int			rmid;

		/* use volatile pointer to prevent code rearrangement */
		volatile XLogCtlData *xlogctl = XLogCtl;

		/*
		 * Update pg_control to show that we are recovering and to show the
		 * selected checkpoint as the place we are starting from. We also mark
		 * pg_control with any minimum recovery stop point obtained from a
		 * backup history file.
		 */
		dbstate_at_startup = ControlFile->state;
		if (InArchiveRecovery)
			ControlFile->state = DB_IN_ARCHIVE_RECOVERY;
		else
		{
			ereport(LOG,
					(errmsg("database system was not properly shut down; "
							"automatic recovery in progress")));
			if (recoveryTargetTLI > ControlFile->checkPointCopy.ThisTimeLineID)
				ereport(LOG,
						(errmsg("crash recovery starts in timeline %u "
								"and has target timeline %u",
								ControlFile->checkPointCopy.ThisTimeLineID,
								recoveryTargetTLI)));
			ControlFile->state = DB_IN_CRASH_RECOVERY;
		}
		ControlFile->prevCheckPoint = ControlFile->checkPoint;
		ControlFile->checkPoint = checkPointLoc;
		ControlFile->checkPointCopy = checkPoint;
		if (InArchiveRecovery)
		{
			/* initialize minRecoveryPoint if not set yet */
			if (ControlFile->minRecoveryPoint < checkPoint.redo)
			{
				ControlFile->minRecoveryPoint = checkPoint.redo;
				ControlFile->minRecoveryPointTLI = checkPoint.ThisTimeLineID;
			}
		}

		/*
		 * Set backupStartPoint if we're starting recovery from a base backup.
		 *
		 * Set backupEndPoint and use minRecoveryPoint as the backup end
		 * location if we're starting recovery from a base backup which was
		 * taken from the standby. In this case, the database system status in
		 * pg_control must indicate DB_IN_ARCHIVE_RECOVERY. If not, which
		 * means that backup is corrupted, so we cancel recovery.
		 */
		if (haveBackupLabel)
		{
			ControlFile->backupStartPoint = checkPoint.redo;
			ControlFile->backupEndRequired = backupEndRequired;

			if (backupFromStandby)
			{
				if (dbstate_at_startup != DB_IN_ARCHIVE_RECOVERY)
					ereport(FATAL,
							(errmsg("backup_label contains data inconsistent with control file"),
							 errhint("This means that the backup is corrupted and you will "
							   "have to use another backup for recovery.")));
				ControlFile->backupEndPoint = ControlFile->minRecoveryPoint;
			}
		}
		ControlFile->time = (pg_time_t) time(NULL);
		/* No need to hold ControlFileLock yet, we aren't up far enough */
		UpdateControlFile();

		/* initialize our local copy of minRecoveryPoint */
		minRecoveryPoint = ControlFile->minRecoveryPoint;
		minRecoveryPointTLI = ControlFile->minRecoveryPointTLI;

		/*
		 * Reset pgstat data, because it may be invalid after recovery.
		 */
		pgstat_reset_all();

		/*
		 * If there was a backup label file, it's done its job and the info
		 * has now been propagated into pg_control.  We must get rid of the
		 * label file so that if we crash during recovery, we'll pick up at
		 * the latest recovery restartpoint instead of going all the way back
		 * to the backup start point.  It seems prudent though to just rename
		 * the file out of the way rather than delete it completely.
		 */
		if (haveBackupLabel)
		{
			unlink(BACKUP_LABEL_OLD);
			if (rename(BACKUP_LABEL_FILE, BACKUP_LABEL_OLD) != 0)
				ereport(FATAL,
						(errcode_for_file_access(),
						 errmsg("could not rename file \"%s\" to \"%s\": %m",
								BACKUP_LABEL_FILE, BACKUP_LABEL_OLD)));
		}

		/* Check that the GUCs used to generate the WAL allow recovery */
		CheckRequiredParameterValues();

		/*
		 * We're in recovery, so unlogged relations may be trashed and must be
		 * reset.  This should be done BEFORE allowing Hot Standby
		 * connections, so that read-only backends don't try to read whatever
		 * garbage is left over from before.
		 */
		ResetUnloggedRelations(UNLOGGED_RELATION_CLEANUP);

		/*
		 * Likewise, delete any saved transaction snapshot files that got left
		 * behind by crashed backends.
		 */
		DeleteAllExportedSnapshotFiles();

		/*
		 * Initialize for Hot Standby, if enabled. We won't let backends in
		 * yet, not until we've reached the min recovery point specified in
		 * control file and we've established a recovery snapshot from a
		 * running-xacts WAL record.
		 */
		if (ArchiveRecoveryRequested && EnableHotStandby)
		{
			TransactionId *xids;
			int			nxids;

			ereport(DEBUG1,
					(errmsg("initializing for hot standby")));

			InitRecoveryTransactionEnvironment();

			if (wasShutdown)
				oldestActiveXID = PrescanPreparedTransactions(&xids, &nxids);
			else
				oldestActiveXID = checkPoint.oldestActiveXid;
			Assert(TransactionIdIsValid(oldestActiveXID));

			/* Tell procarray about the range of xids it has to deal with */
			ProcArrayInitRecovery(ShmemVariableCache->nextXid);

			/*
			 * Startup commit log and subtrans only. MultiXact has already
			 * been started up and other SLRUs are not maintained during
			 * recovery and need not be started yet.
			 */
			StartupCLOG();
			StartupSUBTRANS(oldestActiveXID);

			/*
			 * If we're beginning at a shutdown checkpoint, we know that
			 * nothing was running on the master at this point. So fake-up an
			 * empty running-xacts record and use that here and now. Recover
			 * additional standby state for prepared transactions.
			 */
			if (wasShutdown)
			{
				RunningTransactionsData running;
				TransactionId latestCompletedXid;

				/*
				 * Construct a RunningTransactions snapshot representing a
				 * shut down server, with only prepared transactions still
				 * alive. We're never overflowed at this point because all
				 * subxids are listed with their parent prepared transactions.
				 */
				running.xcnt = nxids;
				running.subxcnt = 0;
				running.subxid_overflow = false;
				running.nextXid = checkPoint.nextXid;
				running.oldestRunningXid = oldestActiveXID;
				latestCompletedXid = checkPoint.nextXid;
				TransactionIdRetreat(latestCompletedXid);
				Assert(TransactionIdIsNormal(latestCompletedXid));
				running.latestCompletedXid = latestCompletedXid;
				running.xids = xids;

				ProcArrayApplyRecoveryInfo(&running);

				StandbyRecoverPreparedTransactions(false);
			}
		}

		/* Initialize resource managers */
		for (rmid = 0; rmid <= RM_MAX_ID; rmid++)
		{
			if (RmgrTable[rmid].rm_startup != NULL)
				RmgrTable[rmid].rm_startup();
		}

		/*
		 * Initialize shared variables for tracking progress of WAL replay,
		 * as if we had just replayed the record before the REDO location.
		 */
		SpinLockAcquire(&xlogctl->info_lck);
		xlogctl->replayEndRecPtr = checkPoint.redo;
		xlogctl->replayEndTLI = ThisTimeLineID;
		xlogctl->lastReplayedEndRecPtr = checkPoint.redo;
		xlogctl->lastReplayedTLI = ThisTimeLineID;
		xlogctl->recoveryLastXTime = 0;
		xlogctl->currentChunkStartTime = 0;
		xlogctl->recoveryPause = false;
		SpinLockRelease(&xlogctl->info_lck);

		/* Also ensure XLogReceiptTime has a sane value */
		XLogReceiptTime = GetCurrentTimestamp();

		/*
		 * Let postmaster know we've started redo now, so that it can launch
		 * checkpointer to perform restartpoints.  We don't bother during
		 * crash recovery as restartpoints can only be performed during
		 * archive recovery.  And we'd like to keep crash recovery simple, to
		 * avoid introducing bugs that could affect you when recovering after
		 * crash.
		 *
		 * After this point, we can no longer assume that we're the only
		 * process in addition to postmaster!  Also, fsync requests are
		 * subsequently to be handled by the checkpointer, not locally.
		 */
		if (ArchiveRecoveryRequested && IsUnderPostmaster)
		{
			PublishStartupProcessInformation();
			SetForwardFsyncRequests();
			SendPostmasterSignal(PMSIGNAL_RECOVERY_STARTED);
			bgwriterLaunched = true;
		}

		/*
		 * Allow read-only connections immediately if we're consistent
		 * already.
		 */
		CheckRecoveryConsistency();

		/*
		 * Find the first record that logically follows the checkpoint --- it
		 * might physically precede it, though.
		 */
		if (checkPoint.redo < RecPtr)
		{
			/* back up to find the record */
			record = ReadRecord(xlogreader, checkPoint.redo, PANIC, false);
		}
		else
		{
			/* just have to read next record after CheckPoint */
			record = ReadRecord(xlogreader, InvalidXLogRecPtr, LOG, false);
		}

		if (record != NULL)
		{
			ErrorContextCallback errcallback;
			TimestampTz xtime;

			InRedo = true;

			ereport(LOG,
					(errmsg("redo starts at %X/%X",
						 (uint32) (ReadRecPtr >> 32), (uint32) ReadRecPtr)));

			/*
			 * main redo apply loop
			 */
			do
			{
				bool		switchedTLI = false;

#ifdef WAL_DEBUG
				if (XLOG_DEBUG ||
				 (rmid == RM_XACT_ID && trace_recovery_messages <= DEBUG2) ||
					(rmid != RM_XACT_ID && trace_recovery_messages <= DEBUG3))
				{
					StringInfoData buf;

					initStringInfo(&buf);
					appendStringInfo(&buf, "REDO @ %X/%X; LSN %X/%X: ",
							(uint32) (ReadRecPtr >> 32), (uint32) ReadRecPtr,
							 (uint32) (EndRecPtr >> 32), (uint32) EndRecPtr);
					xlog_outrec(&buf, record);
					appendStringInfoString(&buf, " - ");
					RmgrTable[record->xl_rmid].rm_desc(&buf,
													   record->xl_info,
													 XLogRecGetData(record));
					elog(LOG, "%s", buf.data);
					pfree(buf.data);
				}
#endif

				/* Handle interrupt signals of startup process */
				HandleStartupProcInterrupts();

				/*
				 * Pause WAL replay, if requested by a hot-standby session via
				 * SetRecoveryPause().
				 *
				 * Note that we intentionally don't take the info_lck spinlock
				 * here.  We might therefore read a slightly stale value of
				 * the recoveryPause flag, but it can't be very stale (no
				 * worse than the last spinlock we did acquire).  Since a
				 * pause request is a pretty asynchronous thing anyway,
				 * possibly responding to it one WAL record later than we
				 * otherwise would is a minor issue, so it doesn't seem worth
				 * adding another spinlock cycle to prevent that.
				 */
				if (xlogctl->recoveryPause)
					recoveryPausesHere();

				/*
				 * Have we reached our recovery target?
				 */
				if (recoveryStopsBefore(record))
				{
					reachedStopPoint = true;	/* see below */
					break;
				}

				/*
				 * If we've been asked to lag the master, wait on
				 * latch until enough time has passed.
				 */
				if (recoveryApplyDelay(record))
				{
					/*
					 * We test for paused recovery again here. If
					 * user sets delayed apply, it may be because
					 * they expect to pause recovery in case of
					 * problems, so we must test again here otherwise
					 * pausing during the delay-wait wouldn't work.
					 */
					if (xlogctl->recoveryPause)
						recoveryPausesHere();
				}

				/* Setup error traceback support for ereport() */
				errcallback.callback = rm_redo_error_callback;
				errcallback.arg = (void *) record;
				errcallback.previous = error_context_stack;
				error_context_stack = &errcallback;

				/*
				 * ShmemVariableCache->nextXid must be beyond record's xid.
				 *
				 * We don't expect anyone else to modify nextXid, hence we
				 * don't need to hold a lock while examining it.  We still
				 * acquire the lock to modify it, though.
				 */
				if (TransactionIdFollowsOrEquals(record->xl_xid,
												 ShmemVariableCache->nextXid))
				{
					LWLockAcquire(XidGenLock, LW_EXCLUSIVE);
					ShmemVariableCache->nextXid = record->xl_xid;
					TransactionIdAdvance(ShmemVariableCache->nextXid);
					LWLockRelease(XidGenLock);
				}

				/*
				 * Before replaying this record, check if this record causes
				 * the current timeline to change. The record is already
				 * considered to be part of the new timeline, so we update
				 * ThisTimeLineID before replaying it. That's important so
				 * that replayEndTLI, which is recorded as the minimum
				 * recovery point's TLI if recovery stops after this record,
				 * is set correctly.
				 */
				if (record->xl_rmid == RM_XLOG_ID)
				{
					TimeLineID	newTLI = ThisTimeLineID;
					TimeLineID	prevTLI = ThisTimeLineID;
					uint8		info = record->xl_info & ~XLR_INFO_MASK;

					if (info == XLOG_CHECKPOINT_SHUTDOWN)
					{
						CheckPoint	checkPoint;

						memcpy(&checkPoint, XLogRecGetData(record), sizeof(CheckPoint));
						newTLI = checkPoint.ThisTimeLineID;
						prevTLI = checkPoint.PrevTimeLineID;
					}
					else if (info == XLOG_END_OF_RECOVERY)
					{
						xl_end_of_recovery xlrec;

						memcpy(&xlrec, XLogRecGetData(record), sizeof(xl_end_of_recovery));
						newTLI = xlrec.ThisTimeLineID;
						prevTLI = xlrec.PrevTimeLineID;
					}

					if (newTLI != ThisTimeLineID)
					{
						/* Check that it's OK to switch to this TLI */
						checkTimeLineSwitch(EndRecPtr, newTLI, prevTLI);

						/* Following WAL records should be run with new TLI */
						ThisTimeLineID = newTLI;
						switchedTLI = true;
					}
				}

				/*
				 * Update shared replayEndRecPtr before replaying this record,
				 * so that XLogFlush will update minRecoveryPoint correctly.
				 */
				SpinLockAcquire(&xlogctl->info_lck);
				xlogctl->replayEndRecPtr = EndRecPtr;
				xlogctl->replayEndTLI = ThisTimeLineID;
				SpinLockRelease(&xlogctl->info_lck);

				/*
				 * If we are attempting to enter Hot Standby mode, process
				 * XIDs we see
				 */
				if (standbyState >= STANDBY_INITIALIZED &&
					TransactionIdIsValid(record->xl_xid))
					RecordKnownAssignedTransactionIds(record->xl_xid);

				/* Now apply the WAL record itself */
				RmgrTable[record->xl_rmid].rm_redo(EndRecPtr, record);

				/* Pop the error context stack */
				error_context_stack = errcallback.previous;

				/*
				 * Update lastReplayedEndRecPtr after this record has been
				 * successfully replayed.
				 */
				SpinLockAcquire(&xlogctl->info_lck);
				xlogctl->lastReplayedEndRecPtr = EndRecPtr;
				xlogctl->lastReplayedTLI = ThisTimeLineID;
				SpinLockRelease(&xlogctl->info_lck);

				/* Remember this record as the last-applied one */
				LastRec = ReadRecPtr;

				/* Allow read-only connections if we're consistent now */
				CheckRecoveryConsistency();

				/*
				 * If this record was a timeline switch, wake up any
				 * walsenders to notice that we are on a new timeline.
				 */
				if (switchedTLI && AllowCascadeReplication())
					WalSndWakeup();

				/* Exit loop if we reached inclusive recovery target */
				if (recoveryStopsAfter(record))
				{
					reachedStopPoint = true;
					break;
				}

				/* Else, try to fetch the next WAL record */
				record = ReadRecord(xlogreader, InvalidXLogRecPtr, LOG, false);
			} while (record != NULL);

			/*
			 * end of main redo apply loop
			 */

			if (recoveryPauseAtTarget && reachedStopPoint)
			{
				SetRecoveryPause(true);
				recoveryPausesHere();
			}

			ereport(LOG,
					(errmsg("redo done at %X/%X",
						 (uint32) (ReadRecPtr >> 32), (uint32) ReadRecPtr)));
			xtime = GetLatestXTime();
			if (xtime)
				ereport(LOG,
					 (errmsg("last completed transaction was at log time %s",
							 timestamptz_to_str(xtime))));
			InRedo = false;
		}
		else
		{
			/* there are no WAL records following the checkpoint */
			ereport(LOG,
					(errmsg("redo is not required")));
		}
	}

	/*
	 * Kill WAL receiver, if it's still running, before we continue to write
	 * the startup checkpoint record. It will trump over the checkpoint and
	 * subsequent records if it's still alive when we start writing WAL.
	 */
	ShutdownWalRcv();

	/*
	 * We don't need the latch anymore. It's not strictly necessary to disown
	 * it, but let's do it for the sake of tidiness.
	 */
	if (StandbyModeRequested)
		DisownLatch(&XLogCtl->recoveryWakeupLatch);

	/*
	 * We are now done reading the xlog from stream. Turn off streaming
	 * recovery to force fetching the files (which would be required at end of
	 * recovery, e.g., timeline history file) from archive or pg_xlog.
	 */
	StandbyMode = false;

	/*
	 * Re-fetch the last valid or last applied record, so we can identify the
	 * exact endpoint of what we consider the valid portion of WAL.
	 */
	record = ReadRecord(xlogreader, LastRec, PANIC, false);
	EndOfLog = EndRecPtr;
	XLByteToPrevSeg(EndOfLog, endLogSegNo);

	/*
	 * Complain if we did not roll forward far enough to render the backup
	 * dump consistent.  Note: it is indeed okay to look at the local variable
	 * minRecoveryPoint here, even though ControlFile->minRecoveryPoint might
	 * be further ahead --- ControlFile->minRecoveryPoint cannot have been
	 * advanced beyond the WAL we processed.
	 */
	if (InRecovery &&
		(EndOfLog < minRecoveryPoint ||
		 !XLogRecPtrIsInvalid(ControlFile->backupStartPoint)))
	{
		if (reachedStopPoint)
		{
			/* stopped because of stop request */
			ereport(FATAL,
					(errmsg("requested recovery stop point is before consistent recovery point")));
		}

		/*
		 * Ran off end of WAL before reaching end-of-backup WAL record, or
		 * minRecoveryPoint. That's usually a bad sign, indicating that you
		 * tried to recover from an online backup but never called
		 * pg_stop_backup(), or you didn't archive all the WAL up to that
		 * point. However, this also happens in crash recovery, if the system
		 * crashes while an online backup is in progress. We must not treat
		 * that as an error, or the database will refuse to start up.
		 */
		if (ArchiveRecoveryRequested || ControlFile->backupEndRequired)
		{
			if (ControlFile->backupEndRequired)
				ereport(FATAL,
						(errmsg("WAL ends before end of online backup"),
						 errhint("All WAL generated while online backup was taken must be available at recovery.")));
			else if (!XLogRecPtrIsInvalid(ControlFile->backupStartPoint))
				ereport(FATAL,
						(errmsg("WAL ends before end of online backup"),
						 errhint("Online backup started with pg_start_backup() must be ended with pg_stop_backup(), and all WAL up to that point must be available at recovery.")));
			else
				ereport(FATAL,
					  (errmsg("WAL ends before consistent recovery point")));
		}
	}

	/*
	 * Consider whether we need to assign a new timeline ID.
	 *
	 * If we are doing an archive recovery, we always assign a new ID.	This
	 * handles a couple of issues.	If we stopped short of the end of WAL
	 * during recovery, then we are clearly generating a new timeline and must
	 * assign it a unique new ID.  Even if we ran to the end, modifying the
	 * current last segment is problematic because it may result in trying to
	 * overwrite an already-archived copy of that segment, and we encourage
	 * DBAs to make their archive_commands reject that.  We can dodge the
	 * problem by making the new active segment have a new timeline ID.
	 *
	 * In a normal crash recovery, we can just extend the timeline we were in.
	 */
	PrevTimeLineID = ThisTimeLineID;
	if (ArchiveRecoveryRequested)
	{
		char		reason[200];

		Assert(InArchiveRecovery);

		ThisTimeLineID = findNewestTimeLine(recoveryTargetTLI) + 1;
		ereport(LOG,
				(errmsg("selected new timeline ID: %u", ThisTimeLineID)));

		/*
		 * Create a comment for the history file to explain why and where
		 * timeline changed.
		 */
		if (recoveryTarget == RECOVERY_TARGET_XID)
			snprintf(reason, sizeof(reason),
					 "%s transaction %u",
					 recoveryStopAfter ? "after" : "before",
					 recoveryStopXid);
		else if (recoveryTarget == RECOVERY_TARGET_TIME)
			snprintf(reason, sizeof(reason),
					 "%s %s\n",
					 recoveryStopAfter ? "after" : "before",
					 timestamptz_to_str(recoveryStopTime));
		else if (recoveryTarget == RECOVERY_TARGET_NAME)
			snprintf(reason, sizeof(reason),
					 "at restore point \"%s\"",
					 recoveryStopName);
		else if (recoveryTarget == RECOVERY_TARGET_IMMEDIATE)
			snprintf(reason, sizeof(reason), "reached consistency");
		else
			snprintf(reason, sizeof(reason), "no recovery target specified");

		writeTimeLineHistory(ThisTimeLineID, recoveryTargetTLI,
							 EndRecPtr, reason);
	}

	/* Save the selected TimeLineID in shared memory, too */
	XLogCtl->ThisTimeLineID = ThisTimeLineID;
	XLogCtl->PrevTimeLineID = PrevTimeLineID;

	/*
	 * We are now done reading the old WAL.  Turn off archive fetching if it
	 * was active, and make a writable copy of the last WAL segment. (Note
	 * that we also have a copy of the last block of the old WAL in readBuf;
	 * we will use that below.)
	 */
	if (ArchiveRecoveryRequested)
		exitArchiveRecovery(xlogreader->readPageTLI, endLogSegNo);

	/*
	 * Prepare to write WAL starting at EndOfLog position, and init xlog
	 * buffer cache using the block containing the last record from the
	 * previous incarnation.
	 */
	openLogSegNo = endLogSegNo;
	openLogFile = XLogFileOpen(openLogSegNo);
	openLogOff = 0;
	Insert = &XLogCtl->Insert;
	Insert->PrevBytePos = XLogRecPtrToBytePos(LastRec);
	Insert->CurrBytePos = XLogRecPtrToBytePos(EndOfLog);

	/*
	 * Tricky point here: readBuf contains the *last* block that the LastRec
	 * record spans, not the one it starts in.	The last block is indeed the
	 * one we want to use.
	 */
	if (EndOfLog % XLOG_BLCKSZ != 0)
	{
		char	   *page;
		int			len;
		int			firstIdx;
		XLogRecPtr	pageBeginPtr;

		pageBeginPtr = EndOfLog - (EndOfLog % XLOG_BLCKSZ);
		Assert(readOff == pageBeginPtr % XLogSegSize);

		firstIdx = XLogRecPtrToBufIdx(EndOfLog);

		/* Copy the valid part of the last block, and zero the rest */
		page = &XLogCtl->pages[firstIdx * XLOG_BLCKSZ];
		len = EndOfLog % XLOG_BLCKSZ;
		memcpy(page, xlogreader->readBuf, len);
		memset(page + len, 0, XLOG_BLCKSZ - len);

		XLogCtl->xlblocks[firstIdx] = pageBeginPtr + XLOG_BLCKSZ;
		XLogCtl->InitializedUpTo = pageBeginPtr + XLOG_BLCKSZ;
	}
	else
	{
		/*
		 * There is no partial block to copy. Just set InitializedUpTo,
		 * and let the first attempt to insert a log record to initialize
		 * the next buffer.
		 */
		XLogCtl->InitializedUpTo = EndOfLog;
	}

	LogwrtResult.Write = LogwrtResult.Flush = EndOfLog;

	XLogCtl->LogwrtResult = LogwrtResult;

	XLogCtl->LogwrtRqst.Write = EndOfLog;
	XLogCtl->LogwrtRqst.Flush = EndOfLog;

	/* Pre-scan prepared transactions to find out the range of XIDs present */
	oldestActiveXID = PrescanPreparedTransactions(NULL, NULL);

	/*
	 * Update full_page_writes in shared memory and write an XLOG_FPW_CHANGE
	 * record before resource manager writes cleanup WAL records or checkpoint
	 * record is written.
	 */
	Insert->fullPageWrites = lastFullPageWrites;
	LocalSetXLogInsertAllowed();
	UpdateFullPageWrites();
	LocalXLogInsertAllowed = -1;

	if (InRecovery)
	{
		int			rmid;

		/*
		 * Resource managers might need to write WAL records, eg, to record
		 * index cleanup actions.  So temporarily enable XLogInsertAllowed in
		 * this process only.
		 */
		LocalSetXLogInsertAllowed();

		/*
		 * Allow resource managers to do any required cleanup.
		 */
		for (rmid = 0; rmid <= RM_MAX_ID; rmid++)
		{
			if (RmgrTable[rmid].rm_cleanup != NULL)
				RmgrTable[rmid].rm_cleanup();
		}

		/* Disallow XLogInsert again */
		LocalXLogInsertAllowed = -1;

		/*
		 * Perform a checkpoint to update all our recovery activity to disk.
		 *
		 * Note that we write a shutdown checkpoint rather than an on-line
		 * one. This is not particularly critical, but since we may be
		 * assigning a new TLI, using a shutdown checkpoint allows us to have
		 * the rule that TLI only changes in shutdown checkpoints, which
		 * allows some extra error checking in xlog_redo.
		 *
		 * In fast promotion, only create a lightweight end-of-recovery record
		 * instead of a full checkpoint. A checkpoint is requested later,
		 * after we're fully out of recovery mode and already accepting
		 * queries.
		 */
		if (bgwriterLaunched)
		{
			if (fast_promote)
			{
				checkPointLoc = ControlFile->prevCheckPoint;

				/*
				 * Confirm the last checkpoint is available for us to recover
				 * from if we fail. Note that we don't check for the secondary
				 * checkpoint since that isn't available in most base backups.
				 */
				record = ReadCheckpointRecord(xlogreader, checkPointLoc, 1, false);
				if (record != NULL)
				{
					fast_promoted = true;

					/*
					 * Insert a special WAL record to mark the end of
					 * recovery, since we aren't doing a checkpoint. That
					 * means that the checkpointer process may likely be in
					 * the middle of a time-smoothed restartpoint and could
					 * continue to be for minutes after this. That sounds
					 * strange, but the effect is roughly the same and it
					 * would be stranger to try to come out of the
					 * restartpoint and then checkpoint. We request a
					 * checkpoint later anyway, just for safety.
					 */
					CreateEndOfRecoveryRecord();
				}
			}

			if (!fast_promoted)
				RequestCheckpoint(CHECKPOINT_END_OF_RECOVERY |
								  CHECKPOINT_IMMEDIATE |
								  CHECKPOINT_WAIT);
		}
		else
			CreateCheckPoint(CHECKPOINT_END_OF_RECOVERY | CHECKPOINT_IMMEDIATE);

		/*
		 * And finally, execute the recovery_end_command, if any.
		 */
		if (recoveryEndCommand)
			ExecuteRecoveryCommand(recoveryEndCommand,
								   "recovery_end_command",
								   true);
	}

	/*
	 * Preallocate additional log files, if wanted.
	 */
	PreallocXlogFiles(EndOfLog);

	/*
	 * Reset initial contents of unlogged relations.  This has to be done
	 * AFTER recovery is complete so that any unlogged relations created
	 * during recovery also get picked up.
	 */
	if (InRecovery)
		ResetUnloggedRelations(UNLOGGED_RELATION_INIT);

	/*
	 * Okay, we're officially UP.
	 */
	InRecovery = false;

	LWLockAcquire(ControlFileLock, LW_EXCLUSIVE);
	ControlFile->state = DB_IN_PRODUCTION;
	ControlFile->time = (pg_time_t) time(NULL);
	UpdateControlFile();
	LWLockRelease(ControlFileLock);

	/* start the archive_timeout timer running */
	XLogCtl->lastSegSwitchTime = (pg_time_t) time(NULL);

	/* also initialize latestCompletedXid, to nextXid - 1 */
	LWLockAcquire(ProcArrayLock, LW_EXCLUSIVE);
	ShmemVariableCache->latestCompletedXid = ShmemVariableCache->nextXid;
	TransactionIdRetreat(ShmemVariableCache->latestCompletedXid);
	LWLockRelease(ProcArrayLock);

	/*
	 * Start up the commit log and subtrans, if not already done for hot
	 * standby.
	 */
	if (standbyState == STANDBY_DISABLED)
	{
		StartupCLOG();
		StartupSUBTRANS(oldestActiveXID);
	}

	/*
	 * Perform end of recovery actions for any SLRUs that need it.
	 */
	TrimCLOG();
	TrimMultiXact();

	/* Reload shared-memory state for prepared transactions */
	RecoverPreparedTransactions();

	/*
	 * Shutdown the recovery environment. This must occur after
	 * RecoverPreparedTransactions(), see notes for lock_twophase_recover()
	 */
	if (standbyState != STANDBY_DISABLED)
		ShutdownRecoveryTransactionEnvironment();

	/* Shut down xlogreader */
	if (readFile >= 0)
	{
		close(readFile);
		readFile = -1;
	}
	XLogReaderFree(xlogreader);

	/*
	 * If any of the critical GUCs have changed, log them before we allow
	 * backends to write WAL.
	 */
	LocalSetXLogInsertAllowed();
	XLogReportParameters();

	/*
	 * All done.  Allow backends to write WAL.	(Although the bool flag is
	 * probably atomic in itself, we use the info_lck here to ensure that
	 * there are no race conditions concerning visibility of other recent
	 * updates to shared memory.)
	 */
	{
		/* use volatile pointer to prevent code rearrangement */
		volatile XLogCtlData *xlogctl = XLogCtl;

		SpinLockAcquire(&xlogctl->info_lck);
		xlogctl->SharedRecoveryInProgress = false;
		SpinLockRelease(&xlogctl->info_lck);
	}

	/*
	 * If there were cascading standby servers connected to us, nudge any wal
	 * sender processes to notice that we've been promoted.
	 */
	WalSndWakeup();

	/*
	 * If this was a fast promotion, request an (online) checkpoint now. This
	 * isn't required for consistency, but the last restartpoint might be far
	 * back, and in case of a crash, recovering from it might take a longer
	 * than is appropriate now that we're not in standby mode anymore.
	 */
	if (fast_promoted)
		RequestCheckpoint(CHECKPOINT_FORCE);
}