tparm_proto(const char *string,
	    TPARM_ARG a1,
	    TPARM_ARG a2,
	    TPARM_ARG a3,
	    TPARM_ARG a4,
	    TPARM_ARG a5,
	    TPARM_ARG a6,
	    TPARM_ARG a7,
	    TPARM_ARG a8,
	    TPARM_ARG a9)
{
    return tparm_varargs(string, a1, a2, a3, a4, a5, a6, a7, a8, a9);
}