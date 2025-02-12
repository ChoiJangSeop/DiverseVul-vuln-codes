horAcc8(TIFF* tif, uint8* cp0, tmsize_t cc)
{
	tmsize_t stride = PredictorState(tif)->stride;

	unsigned char* cp = (unsigned char*) cp0;
	assert((cc%stride)==0);
	if (cc > stride) {
		/*
		 * Pipeline the most common cases.
		 */
		if (stride == 3)  {
			unsigned int cr = cp[0];
			unsigned int cg = cp[1];
			unsigned int cb = cp[2];
			cc -= 3;
			cp += 3;
			while (cc>0) {
				cp[0] = (unsigned char) ((cr += cp[0]) & 0xff);
				cp[1] = (unsigned char) ((cg += cp[1]) & 0xff);
				cp[2] = (unsigned char) ((cb += cp[2]) & 0xff);
				cc -= 3;
				cp += 3;
			}
		} else if (stride == 4)  {
			unsigned int cr = cp[0];
			unsigned int cg = cp[1];
			unsigned int cb = cp[2];
			unsigned int ca = cp[3];
			cc -= 4;
			cp += 4;
			while (cc>0) {
				cp[0] = (unsigned char) ((cr += cp[0]) & 0xff);
				cp[1] = (unsigned char) ((cg += cp[1]) & 0xff);
				cp[2] = (unsigned char) ((cb += cp[2]) & 0xff);
				cp[3] = (unsigned char) ((ca += cp[3]) & 0xff);
				cc -= 4;
				cp += 4;
			}
		} else  {
			cc -= stride;
			do {
				REPEAT4(stride, cp[stride] =
					(unsigned char) ((cp[stride] + *cp) & 0xff); cp++)
				cc -= stride;
			} while (cc>0);
		}
	}
}