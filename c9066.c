void cliprdr_free_format_list(CLIPRDR_FORMAT_LIST* formatList)
{
	UINT index = 0;

	if (formatList == NULL)
		return;

	if (formatList->formats)
	{
		for (index = 0; index < formatList->numFormats; index++)
		{
			free(formatList->formats[index].formatName);
		}

		free(formatList->formats);
	}
}