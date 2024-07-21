NCURSES_SP_NAME(mcprint) (NCURSES_SP_DCLx char *data, int len)
/* ship binary character data to the printer via mc4/mc5/mc5p */
{
    int result;
    char *mybuf, *switchon;
    size_t onsize, offsize;
    size_t need;

    errno = 0;
    if (!HasTInfoTerminal(SP_PARM)
	|| len <= 0
	|| (!prtr_non && (!prtr_on || !prtr_off))) {
	errno = ENODEV;
	return (ERR);
    }

    if (prtr_non) {
	switchon = TPARM_1(prtr_non, len);
	onsize = strlen(switchon);
	offsize = 0;
    } else {
	switchon = prtr_on;
	onsize = strlen(prtr_on);
	offsize = strlen(prtr_off);
    }

    need = onsize + (size_t) len + offsize;

    if (switchon == 0
	|| (mybuf = typeMalloc(char, need + 1)) == 0) {
	errno = ENOMEM;
	return (ERR);
    }

    _nc_STRCPY(mybuf, switchon, need);
    memcpy(mybuf + onsize, data, (size_t) len);
    if (offsize)
	_nc_STRCPY(mybuf + onsize + len, prtr_off, need);

    /*
     * We're relying on the atomicity of UNIX writes here.  The
     * danger is that output from a refresh() might get interspersed
     * with the printer data after the write call returns but before the
     * data has actually been shipped to the terminal.  If the write(2)
     * operation is truly atomic we're protected from this.
     */
    result = (int) write(TerminalOf(SP_PARM)->Filedes, mybuf, need);

    /*
     * By giving up our scheduler slot here we increase the odds that the
     * kernel will ship the contiguous clist items from the last write
     * immediately.
     */
#ifndef _WIN32
    (void) sleep(0);
#endif
    free(mybuf);
    return (result);
}