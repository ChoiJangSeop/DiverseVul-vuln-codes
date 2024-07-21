TightDecoder::FilterGradient24(const rdr::U8 *inbuf,
                               const PixelFormat& pf, PIXEL_T* outbuf,
                               int stride, const Rect& r)
{
  int x, y, c;
  rdr::U8 prevRow[TIGHT_MAX_WIDTH*3];
  rdr::U8 thisRow[TIGHT_MAX_WIDTH*3];
  rdr::U8 pix[3]; 
  int est[3]; 

  memset(prevRow, 0, sizeof(prevRow));

  // Set up shortcut variables
  int rectHeight = r.height();
  int rectWidth = r.width();

  for (y = 0; y < rectHeight; y++) {
    /* First pixel in a row */
    for (c = 0; c < 3; c++) {
      pix[c] = inbuf[y*rectWidth*3+c] + prevRow[c];
      thisRow[c] = pix[c];
    }
    pf.bufferFromRGB((rdr::U8*)&outbuf[y*stride], pix, 1);

    /* Remaining pixels of a row */
    for (x = 1; x < rectWidth; x++) {
      for (c = 0; c < 3; c++) {
        est[c] = prevRow[x*3+c] + pix[c] - prevRow[(x-1)*3+c];
        if (est[c] > 0xff) {
          est[c] = 0xff;
        } else if (est[c] < 0) {
          est[c] = 0;
        }
        pix[c] = inbuf[(y*rectWidth+x)*3+c] + est[c];
        thisRow[x*3+c] = pix[c];
      }
      pf.bufferFromRGB((rdr::U8*)&outbuf[y*stride+x], pix, 1);
    }

    memcpy(prevRow, thisRow, sizeof(prevRow));
  }
}