ftp_type(ftpbuf_t *ftp, ftptype_t type)
{
	char typechar[2] = "?";

	if (ftp == NULL) {
		return 0;
	}
	if (type == ftp->type) { 
		return 1;
	}
	if (type == FTPTYPE_ASCII) {
		typechar[0] = 'A';
	} else if (type == FTPTYPE_IMAGE) {
		typechar[0] = 'I';
	} else {
		return 0;
	}
	if (!ftp_putcmd(ftp, "TYPE", typechar)) {
		return 0;
	}
	if (!ftp_getresp(ftp) || ftp->resp != 200) {
		return 0;
	}
	ftp->type = type;

	return 1;
}