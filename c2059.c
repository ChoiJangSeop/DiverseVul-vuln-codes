PHP_FUNCTION(iptcembed)
{
	char *iptcdata, *jpeg_file;
	int iptcdata_len, jpeg_file_len;
	long spool = 0;
	FILE *fp;
	unsigned int marker, done = 0, inx;
	unsigned char *spoolbuf = NULL, *poi = NULL;
	struct stat sb;
	zend_bool written = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|l", &iptcdata, &iptcdata_len, &jpeg_file, &jpeg_file_len, &spool) != SUCCESS) {
		return;
	}

	if (PG(safe_mode) && (!php_checkuid(jpeg_file, NULL, CHECKUID_CHECK_FILE_AND_DIR))) {
		RETURN_FALSE;
	}

	if (php_check_open_basedir(jpeg_file TSRMLS_CC)) {
		RETURN_FALSE;
	}

	if ((fp = VCWD_FOPEN(jpeg_file, "rb")) == 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to open %s", jpeg_file);
		RETURN_FALSE;
	}

	if (spool < 2) {
		fstat(fileno(fp), &sb);

		poi = spoolbuf = safe_emalloc(1, iptcdata_len + sizeof(psheader) + sb.st_size + 1024, 1);
		memset(poi, 0, iptcdata_len + sizeof(psheader) + sb.st_size + 1024 + 1);
	} 

	if (php_iptc_get1(fp, spool, poi?&poi:0 TSRMLS_CC) != 0xFF) {
		fclose(fp);
		if (spoolbuf) {
			efree(spoolbuf);
		}
		RETURN_FALSE;
	}

	if (php_iptc_get1(fp, spool, poi?&poi:0 TSRMLS_CC) != 0xD8) {
		fclose(fp);
		if (spoolbuf) {
			efree(spoolbuf);
		}
		RETURN_FALSE;
	}

	while (!done) {
		marker = php_iptc_next_marker(fp, spool, poi?&poi:0 TSRMLS_CC);

		if (marker == M_EOI) { /* EOF */
			break;
		} else if (marker != M_APP13) { 
			php_iptc_put1(fp, spool, (unsigned char)marker, poi?&poi:0 TSRMLS_CC);
		}

		switch (marker) {
			case M_APP13:
				/* we are going to write a new APP13 marker, so don't output the old one */
				php_iptc_skip_variable(fp, 0, 0 TSRMLS_CC);    
				php_iptc_read_remaining(fp, spool, poi?&poi:0 TSRMLS_CC);
				done = 1;
				break;

			case M_APP0:
				/* APP0 is in each and every JPEG, so when we hit APP0 we insert our new APP13! */
			case M_APP1:
				if (written) {
					/* don't try to write the data twice */
					break;
				}
				written = 1;

				php_iptc_skip_variable(fp, spool, poi?&poi:0 TSRMLS_CC);

				if (iptcdata_len & 1) {
					iptcdata_len++; /* make the length even */
				}

				psheader[ 2 ] = (iptcdata_len+28)>>8;
				psheader[ 3 ] = (iptcdata_len+28)&0xff;

				for (inx = 0; inx < 28; inx++) {
					php_iptc_put1(fp, spool, psheader[inx], poi?&poi:0 TSRMLS_CC);
				}

				php_iptc_put1(fp, spool, (unsigned char)(iptcdata_len>>8), poi?&poi:0 TSRMLS_CC);
				php_iptc_put1(fp, spool, (unsigned char)(iptcdata_len&0xff), poi?&poi:0 TSRMLS_CC);

				for (inx = 0; inx < iptcdata_len; inx++) {
					php_iptc_put1(fp, spool, iptcdata[inx], poi?&poi:0 TSRMLS_CC);
				}
				break;

			case M_SOS:								
				/* we hit data, no more marker-inserting can be done! */
				php_iptc_read_remaining(fp, spool, poi?&poi:0 TSRMLS_CC);
				done = 1;
				break;

			default:
				php_iptc_skip_variable(fp, spool, poi?&poi:0 TSRMLS_CC);
				break;
		}
	}

	fclose(fp);

	if (spool < 2) {
		RETVAL_STRINGL(spoolbuf, poi - spoolbuf, 0);
	} else {
		RETURN_TRUE;
	}
}