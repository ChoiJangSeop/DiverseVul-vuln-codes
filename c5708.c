int sum_end(char *sum)
{
	switch (cursum_type) {
	  case CSUM_MD5:
		md5_result(&md, (uchar *)sum);
		break;
	  case CSUM_MD4:
	  case CSUM_MD4_OLD:
		mdfour_update(&md, (uchar *)md.buffer, sumresidue);
		mdfour_result(&md, (uchar *)sum);
		break;
	  case CSUM_MD4_BUSTED:
		if (sumresidue)
			mdfour_update(&md, (uchar *)md.buffer, sumresidue);
		mdfour_result(&md, (uchar *)sum);
		break;
	  case CSUM_NONE:
		*sum = '\0';
		break;
	}

	return csum_len_for_type(cursum_type);
}