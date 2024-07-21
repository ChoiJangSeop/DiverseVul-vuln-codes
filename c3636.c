const mbfl_encoding *mbfl_encoding_detector_judge2(mbfl_encoding_detector *identd)
{
	mbfl_identify_filter *filter;
	const mbfl_encoding *encoding = NULL;
	int n;

	/* judge */
	if (identd != NULL) {
		n = identd->filter_list_size - 1;
		while (n >= 0) {
			filter = identd->filter_list[n];
			if (!filter->flag) {
				if (!identd->strict || !filter->status) {
					encoding = filter->encoding;
				}
			}
			n--;
		}
 
		/* fallback judge */
		if (!encoding) {
			n = identd->filter_list_size - 1;
			while (n >= 0) {
				filter = identd->filter_list[n];
				if (!filter->flag) {
					encoding = filter->encoding;
				}
				n--;
 			}
		}
	}

	return encoding;
}