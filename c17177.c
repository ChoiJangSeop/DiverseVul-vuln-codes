check_sgr(TERMTYPE2 *tp, char *zero, int num, char *cap, const char *name)
{
    char *test;

    _nc_tparm_err = 0;
    test = TPARM_9(set_attributes,
		   num == 1,
		   num == 2,
		   num == 3,
		   num == 4,
		   num == 5,
		   num == 6,
		   num == 7,
		   num == 8,
		   num == 9);
    if (test != 0) {
	if (PRESENT(cap)) {
	    if (!similar_sgr(num, test, cap)) {
		_nc_warning("%s differs from sgr(%d)\n\t%s=%s\n\tsgr(%d)=%s",
			    name, num,
			    name, _nc_visbuf2(1, cap),
			    num, _nc_visbuf2(2, test));
	    }
	} else if (_nc_capcmp(test, zero)) {
	    _nc_warning("sgr(%d) present, but not %s", num, name);
	}
    } else if (PRESENT(cap)) {
	_nc_warning("sgr(%d) missing, but %s present", num, name);
    }
    if (_nc_tparm_err)
	_nc_warning("stack error in sgr(%d) string", num);
    return test;
}