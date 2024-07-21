mbfl_identify_encoding2(mbfl_string *string, const mbfl_encoding **elist, int elistsz, int strict)
{
	int i, n, num, bad;
	unsigned char *p;
	mbfl_identify_filter *flist, *filter;
	const mbfl_encoding *encoding;

	/* flist is an array of mbfl_identify_filter instances */
	flist = (mbfl_identify_filter *)mbfl_calloc(elistsz, sizeof(mbfl_identify_filter));
	if (flist == NULL) {
		return NULL;
	}

	num = 0;
	if (elist != NULL) {
		for (i = 0; i < elistsz; i++) {
			if (!mbfl_identify_filter_init2(&flist[num], elist[i])) {
				num++;
			}
		}
	}

	/* feed data */
	n = string->len;
	p = string->val;

	if (p != NULL) {
		bad = 0;
		while (n > 0) {
			for (i = 0; i < num; i++) {
				filter = &flist[i];
				if (!filter->flag) {
					(*filter->filter_function)(*p, filter);
					if (filter->flag) {
						bad++;
					}
				}
			}
			if ((num - 1) <= bad && !strict) {
				break;
			}
			p++;
			n--;
		}
	}

	/* judge */
	encoding = NULL;

	for (i = 0; i < num; i++) {
		filter = &flist[i];
		if (!filter->flag) {
			if (strict && filter->status) {
 				continue;
 			}
			encoding = filter->encoding;
			break;
		}
	}

	/* fall-back judge */
	if (!encoding) {
		for (i = 0; i < num; i++) {
			filter = &flist[i];
			if (!filter->flag && (!strict || !filter->status)) {
				encoding = filter->encoding;
				break;
			}
		}
	}
 
	/* cleanup */
	/* dtors should be called in reverse order */
	i = num; while (--i >= 0) {
		mbfl_identify_filter_cleanup(&flist[i]);
	}

	mbfl_free((void *)flist);

	return encoding;
}