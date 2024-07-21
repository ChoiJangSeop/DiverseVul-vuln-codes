DECLAREContigPutFunc(putcontig8bitYCbCr41tile)
{
    (void) y;
    /* XXX adjust fromskew */
    do {
	x = w>>2;
	do {
	    int32 Cb = pp[4];
	    int32 Cr = pp[5];

	    YCbCrtoRGB(cp [0], pp[0]);
	    YCbCrtoRGB(cp [1], pp[1]);
	    YCbCrtoRGB(cp [2], pp[2]);
	    YCbCrtoRGB(cp [3], pp[3]);

	    cp += 4;
	    pp += 6;
	} while (--x);

        if( (w&3) != 0 )
        {
	    int32 Cb = pp[4];
	    int32 Cr = pp[5];

            switch( (w&3) ) {
              case 3: YCbCrtoRGB(cp [2], pp[2]);
              case 2: YCbCrtoRGB(cp [1], pp[1]);
              case 1: YCbCrtoRGB(cp [0], pp[0]);
              case 0: break;
            }

            cp += (w&3);
            pp += 6;
        }

	cp += toskew;
	pp += fromskew;
    } while (--h);

}