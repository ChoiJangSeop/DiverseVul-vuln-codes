static UINT cliprdr_server_receive_capabilities(CliprdrServerContext* context, wStream* s,
                                                const CLIPRDR_HEADER* header)
{
	UINT16 index;
	UINT16 capabilitySetType;
	UINT16 capabilitySetLength;
	UINT error = CHANNEL_RC_OK;
	size_t cap_sets_size = 0;
	CLIPRDR_CAPABILITIES capabilities;
	CLIPRDR_CAPABILITY_SET* capSet;
	void* tmp;

	WINPR_UNUSED(header);

	/* set `capabilitySets` to NULL so `realloc` will know to alloc the first block */
	capabilities.capabilitySets = NULL;

	WLog_DBG(TAG, "CliprdrClientCapabilities");
	Stream_Read_UINT16(s, capabilities.cCapabilitiesSets); /* cCapabilitiesSets (2 bytes) */
	Stream_Seek_UINT16(s);                                 /* pad1 (2 bytes) */

	for (index = 0; index < capabilities.cCapabilitiesSets; index++)
	{
		Stream_Read_UINT16(s, capabilitySetType);   /* capabilitySetType (2 bytes) */
		Stream_Read_UINT16(s, capabilitySetLength); /* capabilitySetLength (2 bytes) */

		cap_sets_size += capabilitySetLength;

		tmp = realloc(capabilities.capabilitySets, cap_sets_size);
		if (tmp == NULL)
		{
			WLog_ERR(TAG, "capabilities.capabilitySets realloc failed!");
			free(capabilities.capabilitySets);
			return CHANNEL_RC_NO_MEMORY;
		}

		capabilities.capabilitySets = (CLIPRDR_CAPABILITY_SET*)tmp;

		capSet = &(capabilities.capabilitySets[index]);

		capSet->capabilitySetType = capabilitySetType;
		capSet->capabilitySetLength = capabilitySetLength;

		switch (capSet->capabilitySetType)
		{
			case CB_CAPSTYPE_GENERAL:
				if ((error = cliprdr_server_receive_general_capability(
				         context, s, (CLIPRDR_GENERAL_CAPABILITY_SET*)capSet)))
				{
					WLog_ERR(TAG,
					         "cliprdr_server_receive_general_capability failed with error %" PRIu32
					         "",
					         error);
					goto out;
				}
				break;

			default:
				WLog_ERR(TAG, "unknown cliprdr capability set: %" PRIu16 "",
				         capSet->capabilitySetType);
				error = ERROR_INVALID_DATA;
				goto out;
		}
	}

	IFCALLRET(context->ClientCapabilities, error, context, &capabilities);
out:
	free(capabilities.capabilitySets);
	return error;
}