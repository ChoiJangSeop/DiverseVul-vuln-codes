DECLAREContigPutFunc(putcontig8bitYCbCr21tile)
{
	(void) y;
	fromskew = (fromskew * 4) / 2;
	do {
		x = w>>1;
		do {
			int32 Cb = pp[2];
			int32 Cr = pp[3];

			YCbCrtoRGB(cp[0], pp[0]);
			YCbCrtoRGB(cp[1], pp[1]);

			cp += 2;
			pp += 4;
		} while (--x);

		if( (w&1) != 0 )
		{
			int32 Cb = pp[2];
			int32 Cr = pp[3];

			YCbCrtoRGB(cp[0], pp[0]);

			cp += 1;
			pp += 4;
		}

		cp += toskew;
		pp += fromskew;
	} while (--h);
}