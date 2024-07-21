expected_params(const char *name)
{
#define DATA(name,count) { { name }, count }
    /* *INDENT-OFF* */
    static const struct {
	const char name[9];
	int count;
    } table[] = {
	DATA( "S0",		1 ),	/* 'screen' extension */
	DATA( "birep",		2 ),
	DATA( "chr",		1 ),
	DATA( "colornm",	1 ),
	DATA( "cpi",		1 ),
	DATA( "csnm",		1 ),
	DATA( "csr",		2 ),
	DATA( "cub",		1 ),
	DATA( "cud",		1 ),
	DATA( "cuf",		1 ),
	DATA( "cup",		2 ),
	DATA( "cuu",		1 ),
	DATA( "cvr",		1 ),
	DATA( "cwin",		5 ),
	DATA( "dch",		1 ),
	DATA( "defc",		3 ),
	DATA( "dial",		1 ),
	DATA( "dispc",		1 ),
	DATA( "dl",		1 ),
	DATA( "ech",		1 ),
	DATA( "getm",		1 ),
	DATA( "hpa",		1 ),
	DATA( "ich",		1 ),
	DATA( "il",		1 ),
	DATA( "indn",		1 ),
	DATA( "initc",		4 ),
	DATA( "initp",		7 ),
	DATA( "lpi",		1 ),
	DATA( "mc5p",		1 ),
	DATA( "mrcup",		2 ),
	DATA( "mvpa",		1 ),
	DATA( "pfkey",		2 ),
	DATA( "pfloc",		2 ),
	DATA( "pfx",		2 ),
	DATA( "pfxl",		3 ),
	DATA( "pln",		2 ),
	DATA( "qdial",		1 ),
	DATA( "rcsd",		1 ),
	DATA( "rep",		2 ),
	DATA( "rin",		1 ),
	DATA( "sclk",		3 ),
	DATA( "scp",		1 ),
	DATA( "scs",		1 ),
	DATA( "scsd",		2 ),
	DATA( "setab",		1 ),
	DATA( "setaf",		1 ),
	DATA( "setb",		1 ),
	DATA( "setcolor",	1 ),
	DATA( "setf",		1 ),
	DATA( "sgr",		9 ),
	DATA( "sgr1",		6 ),
	DATA( "slength",	1 ),
	DATA( "slines",		1 ),
	DATA( "smgbp",		1 ),	/* 2 if smgtp is not given */
	DATA( "smglp",		1 ),
	DATA( "smglr",		2 ),
	DATA( "smgrp",		1 ),
	DATA( "smgtb",		2 ),
	DATA( "smgtp",		1 ),
	DATA( "tsl",		1 ),
	DATA( "u6",		-1 ),
	DATA( "vpa",		1 ),
	DATA( "wind",		4 ),
	DATA( "wingo",		1 ),
    };
    /* *INDENT-ON* */

#undef DATA

    unsigned n;
    int result = 0;		/* function-keys, etc., use none */

    for (n = 0; n < SIZEOF(table); n++) {
	if (!strcmp(name, table[n].name)) {
	    result = table[n].count;
	    break;
	}
    }

    return result;
}