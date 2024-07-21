check_1_infotocap(const char *name, NCURSES_CONST char *value, int count)
{
    int k;
    int ignored;
    long numbers[1 + NUM_PARM];
    char *strings[1 + NUM_PARM];
    char *p_is_s[NUM_PARM];
    char *result;
    char blob[NUM_PARM * 10];
    char *next = blob;

    *next++ = '\0';
    for (k = 1; k <= NUM_PARM; k++) {
	numbers[k] = count;
	_nc_SPRINTF(next,
		    _nc_SLIMIT(sizeof(blob) - (size_t) (next - blob))
		    "XYZ%d", count);
	strings[k] = next;
	next += strlen(next) + 1;
    }

    switch (tparm_type(name)) {
    case Num_Str:
	result = TPARM_2(value, numbers[1], strings[2]);
	break;
    case Num_Str_Str:
	result = TPARM_3(value, numbers[1], strings[2], strings[3]);
	break;
    case Numbers:
    default:
	(void) _nc_tparm_analyze(value, p_is_s, &ignored);
#define myParam(n) (p_is_s[n - 1] != 0 ? ((TPARM_ARG) strings[n]) : numbers[n])
	result = TPARM_9(value,
			 myParam(1),
			 myParam(2),
			 myParam(3),
			 myParam(4),
			 myParam(5),
			 myParam(6),
			 myParam(7),
			 myParam(8),
			 myParam(9));
	break;
    }
    return strdup(result);
}