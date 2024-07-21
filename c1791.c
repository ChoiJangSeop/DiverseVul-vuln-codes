XLogRestorePoint(const char *rpName)
{
	XLogRecPtr	RecPtr;
	XLogRecData rdata;
	xl_restore_point xlrec;

	xlrec.rp_time = GetCurrentTimestamp();
	strncpy(xlrec.rp_name, rpName, MAXFNAMELEN);

	rdata.buffer = InvalidBuffer;
	rdata.data = (char *) &xlrec;
	rdata.len = sizeof(xl_restore_point);
	rdata.next = NULL;

	RecPtr = XLogInsert(RM_XLOG_ID, XLOG_RESTORE_POINT, &rdata);

	ereport(LOG,
			(errmsg("restore point \"%s\" created at %X/%X",
					rpName, (uint32) (RecPtr >> 32), (uint32) RecPtr)));

	return RecPtr;
}