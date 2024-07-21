void get_checksum2(char *buf, int32 len, char *sum)
{
	md_context m;

	if (protocol_version >= 30) {
		uchar seedbuf[4];
		md5_begin(&m);
		md5_update(&m, (uchar *)buf, len);
		if (checksum_seed) {
			SIVALu(seedbuf, 0, checksum_seed);
			md5_update(&m, seedbuf, 4);
		}
		md5_result(&m, (uchar *)sum);
	} else {
		int32 i;
		static char *buf1;
		static int32 len1;

		mdfour_begin(&m);

		if (len > len1) {
			if (buf1)
				free(buf1);
			buf1 = new_array(char, len+4);
			len1 = len;
			if (!buf1)
				out_of_memory("get_checksum2");
		}

		memcpy(buf1, buf, len);
		if (checksum_seed) {
			SIVAL(buf1,len,checksum_seed);
			len += 4;
		}

		for (i = 0; i + CSUM_CHUNK <= len; i += CSUM_CHUNK)
			mdfour_update(&m, (uchar *)(buf1+i), CSUM_CHUNK);

		/*
		 * Prior to version 27 an incorrect MD4 checksum was computed
		 * by failing to call mdfour_tail() for block sizes that
		 * are multiples of 64.  This is fixed by calling mdfour_update()
		 * even when there are no more bytes.
		 */
		if (len - i > 0 || protocol_version >= 27)
			mdfour_update(&m, (uchar *)(buf1+i), len-i);

		mdfour_result(&m, (uchar *)sum);
	}
}