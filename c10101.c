static int do_format(int drive, struct format_descr *tmp_format_req)
{
	int ret;

	if (lock_fdc(drive, true))
		return -EINTR;

	set_floppy(drive);
	if (!_floppy ||
	    _floppy->track > DP->tracks ||
	    tmp_format_req->track >= _floppy->track ||
	    tmp_format_req->head >= _floppy->head ||
	    (_floppy->sect << 2) % (1 << FD_SIZECODE(_floppy)) ||
	    !_floppy->fmt_gap) {
		process_fd_request();
		return -EINVAL;
	}
	format_req = *tmp_format_req;
	format_errors = 0;
	cont = &format_cont;
	errors = &format_errors;
	ret = wait_til_done(redo_format, true);
	if (ret == -EINTR)
		return -EINTR;
	process_fd_request();
	return ret;
}