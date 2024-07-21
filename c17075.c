static UINT cliprdr_server_receive_general_capability(CliprdrServerContext* context, wStream* s,
                                                      CLIPRDR_GENERAL_CAPABILITY_SET* cap_set)
{
	Stream_Read_UINT32(s, cap_set->version);      /* version (4 bytes) */
	Stream_Read_UINT32(s, cap_set->generalFlags); /* generalFlags (4 bytes) */

	if (context->useLongFormatNames)
		context->useLongFormatNames =
		    (cap_set->generalFlags & CB_USE_LONG_FORMAT_NAMES) ? TRUE : FALSE;

	if (context->streamFileClipEnabled)
		context->streamFileClipEnabled =
		    (cap_set->generalFlags & CB_STREAM_FILECLIP_ENABLED) ? TRUE : FALSE;

	if (context->fileClipNoFilePaths)
		context->fileClipNoFilePaths =
		    (cap_set->generalFlags & CB_FILECLIP_NO_FILE_PATHS) ? TRUE : FALSE;

	if (context->canLockClipData)
		context->canLockClipData = (cap_set->generalFlags & CB_CAN_LOCK_CLIPDATA) ? TRUE : FALSE;

	return CHANNEL_RC_OK;
}