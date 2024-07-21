unsigned _gnutls_fips_mode_enabled(void)
{
unsigned f1p = 0, f2p;
FILE* fd;
const char *p;

	if (_fips_mode != -1)
		return _fips_mode;

	p = getenv("GNUTLS_FORCE_FIPS_MODE");
	if (p) {
		if (p[0] == '1')
			_fips_mode = 1;
		else if (p[0] == '2')
			_fips_mode = 2;
		else
			_fips_mode = 0;
		return _fips_mode;
	}

	fd = fopen(FIPS_KERNEL_FILE, "r");
	if (fd != NULL) {
		f1p = fgetc(fd);
		fclose(fd);
		
		if (f1p == '1') f1p = 1;
		else f1p = 0;
	}

	f2p = !access(FIPS_SYSTEM_FILE, F_OK);

	if (f1p != 0 && f2p != 0) {
		_gnutls_debug_log("FIPS140-2 mode enabled\n");
		_fips_mode = 1;
		return _fips_mode;
	}

	if (f2p != 0) {
		/* a funny state where self tests are performed
		 * and ignored */
		_gnutls_debug_log("FIPS140-2 ZOMBIE mode enabled\n");
		_fips_mode = 2;
		return _fips_mode;
	}

	_fips_mode = 0;
	return _fips_mode;
}