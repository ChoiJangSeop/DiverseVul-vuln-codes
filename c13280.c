void CLASS foveon_sd_load_raw()
{
  struct decode *dindex;
  short diff[1024];
  unsigned bitbuf=0;
  int pred[3], row, col, bit=-1, c, i;

  read_shorts ((ushort *) diff, 1024);
  if (!load_flags) foveon_decoder (1024, 0);

  for (row=0; row < height; row++) {
#ifdef LIBRAW_LIBRARY_BUILD
    checkCancel();
#endif
    memset (pred, 0, sizeof pred);
    if (!bit && !load_flags && atoi(model+2) < 14) get4();
    for (col=bit=0; col < width; col++) {
      if (load_flags) {
	bitbuf = get4();
	FORC3 pred[2-c] += diff[bitbuf >> c*10 & 0x3ff];
      }
      else FORC3 {
	for (dindex=first_decode; dindex->branch[0]; ) {
	  if ((bit = (bit-1) & 31) == 31)
	    for (i=0; i < 4; i++)
	      bitbuf = (bitbuf << 8) + fgetc(ifp);
	  dindex = dindex->branch[bitbuf >> bit & 1];
	}
	pred[c] += diff[dindex->leaf];
	if (pred[c] >> 16 && ~pred[c] >> 16) derror();
      }
      FORC3 image[row*width+col][c] = pred[c];
    }
  }
}