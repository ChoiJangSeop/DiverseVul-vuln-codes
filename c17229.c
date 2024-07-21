same_color(NCURSES_CONST char *oldcap, NCURSES_CONST char *newcap, int limit)
{
    bool result = FALSE;
    if (limit > 16)
	limit = 16;
    if (limit >= 8) {
	int n;
	int same;
	for (n = same = 0; n < limit; ++n) {
	    char *oldvalue = strdup(TPARM_1(oldcap, n));
	    char *newvalue = strdup(TPARM_1(newcap, n));
	    same += !strcmp(oldvalue, newvalue);
	    free(oldvalue);
	    free(newvalue);
	}
	result = (same == limit);
    }
    return result;
}