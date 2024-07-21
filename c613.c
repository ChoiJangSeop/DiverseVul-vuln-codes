void ZRLE_ENCODE_TILE(PIXEL_T* data, int w, int h, zrleOutStream* os,
	int zywrle_level, int *zywrleBuf)
{
  /* First find the palette and the number of runs */

  zrlePaletteHelper *ph;

  int runs = 0;
  int singlePixels = 0;

  rfbBool useRle;
  rfbBool usePalette;

  int estimatedBytes;
  int plainRleBytes;
  int i;

  PIXEL_T* ptr = data;
  PIXEL_T* end = ptr + h * w;
  *end = ~*(end-1); /* one past the end is different so the while loop ends */

  ph = &paletteHelper;
  zrlePaletteHelperInit(ph);

  while (ptr < end) {
    PIXEL_T pix = *ptr;
    if (*++ptr != pix) {
      singlePixels++;
    } else {
      while (*++ptr == pix) ;
      runs++;
    }
    zrlePaletteHelperInsert(ph, pix);
  }

  /* Solid tile is a special case */

  if (ph->size == 1) {
    zrleOutStreamWriteU8(os, 1);
    zrleOutStreamWRITE_PIXEL(os, ph->palette[0]);
    return;
  }

  /* Try to work out whether to use RLE and/or a palette.  We do this by
     estimating the number of bytes which will be generated and picking the
     method which results in the fewest bytes.  Of course this may not result
     in the fewest bytes after compression... */

  useRle = FALSE;
  usePalette = FALSE;

  estimatedBytes = w * h * (BPPOUT/8); /* start assuming raw */

#if BPP!=8
  if (zywrle_level > 0 && !(zywrle_level & 0x80))
	  estimatedBytes >>= zywrle_level;
#endif

  plainRleBytes = ((BPPOUT/8)+1) * (runs + singlePixels);

  if (plainRleBytes < estimatedBytes) {
    useRle = TRUE;
    estimatedBytes = plainRleBytes;
  }

  if (ph->size < 128) {
    int paletteRleBytes = (BPPOUT/8) * ph->size + 2 * runs + singlePixels;

    if (paletteRleBytes < estimatedBytes) {
      useRle = TRUE;
      usePalette = TRUE;
      estimatedBytes = paletteRleBytes;
    }

    if (ph->size < 17) {
      int packedBytes = ((BPPOUT/8) * ph->size +
                         w * h * bitsPerPackedPixel[ph->size-1] / 8);

      if (packedBytes < estimatedBytes) {
        useRle = FALSE;
        usePalette = TRUE;
        estimatedBytes = packedBytes;
      }
    }
  }

  if (!usePalette) ph->size = 0;

  zrleOutStreamWriteU8(os, (useRle ? 128 : 0) | ph->size);

  for (i = 0; i < ph->size; i++) {
    zrleOutStreamWRITE_PIXEL(os, ph->palette[i]);
  }

  if (useRle) {

    PIXEL_T* ptr = data;
    PIXEL_T* end = ptr + w * h;
    PIXEL_T* runStart;
    PIXEL_T pix;
    while (ptr < end) {
      int len;
      runStart = ptr;
      pix = *ptr++;
      while (*ptr == pix && ptr < end)
        ptr++;
      len = ptr - runStart;
      if (len <= 2 && usePalette) {
        int index = zrlePaletteHelperLookup(ph, pix);
        if (len == 2)
          zrleOutStreamWriteU8(os, index);
        zrleOutStreamWriteU8(os, index);
        continue;
      }
      if (usePalette) {
        int index = zrlePaletteHelperLookup(ph, pix);
        zrleOutStreamWriteU8(os, index | 128);
      } else {
        zrleOutStreamWRITE_PIXEL(os, pix);
      }
      len -= 1;
      while (len >= 255) {
        zrleOutStreamWriteU8(os, 255);
        len -= 255;
      }
      zrleOutStreamWriteU8(os, len);
    }

  } else {

    /* no RLE */

    if (usePalette) {
      int bppp;
      PIXEL_T* ptr = data;

      /* packed pixels */

      assert (ph->size < 17);

      bppp = bitsPerPackedPixel[ph->size-1];

      for (i = 0; i < h; i++) {
        zrle_U8 nbits = 0;
        zrle_U8 byte = 0;

        PIXEL_T* eol = ptr + w;

        while (ptr < eol) {
          PIXEL_T pix = *ptr++;
          zrle_U8 index = zrlePaletteHelperLookup(ph, pix);
          byte = (byte << bppp) | index;
          nbits += bppp;
          if (nbits >= 8) {
            zrleOutStreamWriteU8(os, byte);
            nbits = 0;
          }
        }
        if (nbits > 0) {
          byte <<= 8 - nbits;
          zrleOutStreamWriteU8(os, byte);
        }
      }
    } else {

      /* raw */

#if BPP!=8
      if (zywrle_level > 0 && !(zywrle_level & 0x80)) {
        ZYWRLE_ANALYZE(data, data, w, h, w, zywrle_level, zywrleBuf);
	ZRLE_ENCODE_TILE(data, w, h, os, zywrle_level | 0x80, zywrleBuf);
      }
      else
#endif
      {
#ifdef CPIXEL
        PIXEL_T *ptr;
        for (ptr = data; ptr < data+w*h; ptr++)
          zrleOutStreamWRITE_PIXEL(os, *ptr);
#else
        zrleOutStreamWriteBytes(os, (zrle_U8 *)data, w*h*(BPP/8));
#endif
      }
    }
  }
}