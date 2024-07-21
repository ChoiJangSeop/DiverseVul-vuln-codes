hufDecode
    (const Int64 * 	hcode,	// i : encoding table
     const HufDec * 	hdecod,	// i : decoding table
     const char* 	in,	// i : compressed input buffer
     int		ni,	// i : input size (in bits)
     int		rlc,	// i : run-length code
     int		no,	// i : expected output size (in bytes)
     unsigned short*	out)	//  o: uncompressed output buffer
{
    Int64 c = 0;
    int lc = 0;
    unsigned short * outb = out;
    unsigned short * oe = out + no;
    const char * ie = in + (ni + 7) / 8; // input byte size

    //
    // Loop on input bytes
    //

    while (in < ie)
    {
	getChar (c, lc, in);

	//
	// Access decoding table
	//

	while (lc >= HUF_DECBITS)
	{
	    const HufDec pl = hdecod[(c >> (lc-HUF_DECBITS)) & HUF_DECMASK];

	    if (pl.len)
	    {
		//
		// Get short code
		//

		lc -= pl.len;
		getCode (pl.lit, rlc, c, lc, in, out, oe);
	    }
	    else
	    {
		if (!pl.p)
		    invalidCode(); // wrong code

		//
		// Search long code
		//

		int j;

		for (j = 0; j < pl.lit; j++)
		{
		    int	l = hufLength (hcode[pl.p[j]]);

		    while (lc < l && in < ie)	// get more bits
			getChar (c, lc, in);

		    if (lc >= l)
		    {
			if (hufCode (hcode[pl.p[j]]) ==
				((c >> (lc - l)) & ((Int64(1) << l) - 1)))
			{
			    //
			    // Found : get long code
			    //

			    lc -= l;
			    getCode (pl.p[j], rlc, c, lc, in, out, oe);
			    break;
			}
		    }
		}

		if (j == pl.lit)
		    invalidCode(); // Not found
	    }
	}
    }

    //
    // Get remaining (short) codes
    //

    int i = (8 - ni) & 7;
    c >>= i;
    lc -= i;

    while (lc > 0)
    {
	const HufDec pl = hdecod[(c << (HUF_DECBITS - lc)) & HUF_DECMASK];

	if (pl.len)
	{
	    lc -= pl.len;
	    getCode (pl.lit, rlc, c, lc, in, out, oe);
	}
	else
	{
	    invalidCode(); // wrong (long) code
	}
    }

    if (out - outb != no)
	notEnoughData ();
}