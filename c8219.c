static int decompTest(char *fileName)
{
  FILE *file = NULL;
  tjhandle handle = NULL;
  unsigned char **jpegBuf = NULL, *srcBuf = NULL;
  unsigned long *jpegSize = NULL, srcSize, totalJpegSize;
  tjtransform *t = NULL;
  double start, elapsed;
  int ps = tjPixelSize[pf], tile, row, col, i, iter, retval = 0, decompsrc = 0;
  char *temp = NULL, tempStr[80], tempStr2[80];
  /* Original image */
  int w = 0, h = 0, tilew, tileh, ntilesw = 1, ntilesh = 1, subsamp = -1,
    cs = -1;
  /* Transformed image */
  int tw, th, ttilew, ttileh, tntilesw, tntilesh, tsubsamp;

  if ((file = fopen(fileName, "rb")) == NULL)
    THROW_UNIX("opening file");
  if (fseek(file, 0, SEEK_END) < 0 ||
      (srcSize = ftell(file)) == (unsigned long)-1)
    THROW_UNIX("determining file size");
  if ((srcBuf = (unsigned char *)malloc(srcSize)) == NULL)
    THROW_UNIX("allocating memory");
  if (fseek(file, 0, SEEK_SET) < 0)
    THROW_UNIX("setting file position");
  if (fread(srcBuf, srcSize, 1, file) < 1)
    THROW_UNIX("reading JPEG data");
  fclose(file);  file = NULL;

  temp = strrchr(fileName, '.');
  if (temp != NULL) *temp = '\0';

  if ((handle = tjInitTransform()) == NULL)
    THROW_TJ("executing tjInitTransform()");
  if (tjDecompressHeader3(handle, srcBuf, srcSize, &w, &h, &subsamp,
                          &cs) == -1)
    THROW_TJ("executing tjDecompressHeader3()");
  if (w < 1 || h < 1)
    THROW("reading JPEG header", "Invalid image dimensions");
  if (cs == TJCS_YCCK || cs == TJCS_CMYK) {
    pf = TJPF_CMYK;  ps = tjPixelSize[pf];
  }

  if (quiet == 1) {
    printf("All performance values in Mpixels/sec\n\n");
    printf("Bitmap     JPEG   JPEG     %s  %s   Xform   Comp    Decomp  ",
           doTile ? "Tile " : "Image", doTile ? "Tile " : "Image");
    if (doYUV) printf("Decode");
    printf("\n");
    printf("Format     CS     Subsamp  Width  Height  Perf    Ratio   Perf    ");
    if (doYUV) printf("Perf");
    printf("\n\n");
  } else if (!quiet)
    printf(">>>>>  JPEG %s --> %s (%s)  <<<<<\n",
           formatName(subsamp, cs, tempStr), pixFormatStr[pf],
           (flags & TJFLAG_BOTTOMUP) ? "Bottom-up" : "Top-down");

  for (tilew = doTile ? 16 : w, tileh = doTile ? 16 : h; ;
       tilew *= 2, tileh *= 2) {
    if (tilew > w) tilew = w;
    if (tileh > h) tileh = h;
    ntilesw = (w + tilew - 1) / tilew;
    ntilesh = (h + tileh - 1) / tileh;

    if ((jpegBuf = (unsigned char **)malloc(sizeof(unsigned char *) *
                                            ntilesw * ntilesh)) == NULL)
      THROW_UNIX("allocating JPEG tile array");
    memset(jpegBuf, 0, sizeof(unsigned char *) * ntilesw * ntilesh);
    if ((jpegSize = (unsigned long *)malloc(sizeof(unsigned long) *
                                            ntilesw * ntilesh)) == NULL)
      THROW_UNIX("allocating JPEG size array");
    memset(jpegSize, 0, sizeof(unsigned long) * ntilesw * ntilesh);

    if ((flags & TJFLAG_NOREALLOC) != 0 || !doTile)
      for (i = 0; i < ntilesw * ntilesh; i++) {
        if ((jpegBuf[i] = (unsigned char *)
                          tjAlloc(tjBufSize(tilew, tileh, subsamp))) == NULL)
          THROW_UNIX("allocating JPEG tiles");
      }

    tw = w;  th = h;  ttilew = tilew;  ttileh = tileh;
    if (!quiet) {
      printf("\n%s size: %d x %d", doTile ? "Tile" : "Image", ttilew, ttileh);
      if (sf.num != 1 || sf.denom != 1)
        printf(" --> %d x %d", TJSCALED(tw, sf), TJSCALED(th, sf));
      printf("\n");
    } else if (quiet == 1) {
      printf("%-4s (%s)  %-5s  %-5s    ", pixFormatStr[pf],
             (flags & TJFLAG_BOTTOMUP) ? "BU" : "TD", csName[cs],
             subNameLong[subsamp]);
      printf("%-5d  %-5d   ", tilew, tileh);
    }

    tsubsamp = subsamp;
    if (doTile || xformOp != TJXOP_NONE || xformOpt != 0 || customFilter) {
      if ((t = (tjtransform *)malloc(sizeof(tjtransform) * ntilesw *
                                     ntilesh)) == NULL)
        THROW_UNIX("allocating image transform array");

      if (xformOp == TJXOP_TRANSPOSE || xformOp == TJXOP_TRANSVERSE ||
          xformOp == TJXOP_ROT90 || xformOp == TJXOP_ROT270) {
        tw = h;  th = w;  ttilew = tileh;  ttileh = tilew;
      }

      if (xformOpt & TJXOPT_GRAY) tsubsamp = TJ_GRAYSCALE;
      if (xformOp == TJXOP_HFLIP || xformOp == TJXOP_ROT180)
        tw = tw - (tw % tjMCUWidth[tsubsamp]);
      if (xformOp == TJXOP_VFLIP || xformOp == TJXOP_ROT180)
        th = th - (th % tjMCUHeight[tsubsamp]);
      if (xformOp == TJXOP_TRANSVERSE || xformOp == TJXOP_ROT90)
        tw = tw - (tw % tjMCUHeight[tsubsamp]);
      if (xformOp == TJXOP_TRANSVERSE || xformOp == TJXOP_ROT270)
        th = th - (th % tjMCUWidth[tsubsamp]);
      tntilesw = (tw + ttilew - 1) / ttilew;
      tntilesh = (th + ttileh - 1) / ttileh;

      if (xformOp == TJXOP_TRANSPOSE || xformOp == TJXOP_TRANSVERSE ||
          xformOp == TJXOP_ROT90 || xformOp == TJXOP_ROT270) {
        if (tsubsamp == TJSAMP_422) tsubsamp = TJSAMP_440;
        else if (tsubsamp == TJSAMP_440) tsubsamp = TJSAMP_422;
      }

      for (row = 0, tile = 0; row < tntilesh; row++) {
        for (col = 0; col < tntilesw; col++, tile++) {
          t[tile].r.w = min(ttilew, tw - col * ttilew);
          t[tile].r.h = min(ttileh, th - row * ttileh);
          t[tile].r.x = col * ttilew;
          t[tile].r.y = row * ttileh;
          t[tile].op = xformOp;
          t[tile].options = xformOpt | TJXOPT_TRIM;
          t[tile].customFilter = customFilter;
          if (t[tile].options & TJXOPT_NOOUTPUT && jpegBuf[tile]) {
            tjFree(jpegBuf[tile]);  jpegBuf[tile] = NULL;
          }
        }
      }

      iter = -1;
      elapsed = 0.;
      while (1) {
        start = getTime();
        if (tjTransform(handle, srcBuf, srcSize, tntilesw * tntilesh, jpegBuf,
                        jpegSize, t, flags) == -1)
          THROW_TJ("executing tjTransform()");
        elapsed += getTime() - start;
        if (iter >= 0) {
          iter++;
          if (elapsed >= benchTime) break;
        } else if (elapsed >= warmup) {
          iter = 0;
          elapsed = 0.;
        }
      }

      free(t);  t = NULL;

      for (tile = 0, totalJpegSize = 0; tile < tntilesw * tntilesh; tile++)
        totalJpegSize += jpegSize[tile];

      if (quiet) {
        printf("%-6s%s%-6s%s",
               sigfig((double)(w * h) / 1000000. / elapsed, 4, tempStr, 80),
               quiet == 2 ? "\n" : "  ",
               sigfig((double)(w * h * ps) / (double)totalJpegSize, 4,
                      tempStr2, 80),
               quiet == 2 ? "\n" : "  ");
      } else if (!quiet) {
        printf("Transform     --> Frame rate:         %f fps\n",
               1.0 / elapsed);
        printf("                  Output image size:  %lu bytes\n",
               totalJpegSize);
        printf("                  Compression ratio:  %f:1\n",
               (double)(w * h * ps) / (double)totalJpegSize);
        printf("                  Throughput:         %f Megapixels/sec\n",
               (double)(w * h) / 1000000. / elapsed);
        printf("                  Output bit stream:  %f Megabits/sec\n",
               (double)totalJpegSize * 8. / 1000000. / elapsed);
      }
    } else {
      if (quiet == 1) printf("N/A     N/A     ");
      tjFree(jpegBuf[0]);
      jpegBuf[0] = NULL;
      decompsrc = 1;
    }

    if (w == tilew) ttilew = tw;
    if (h == tileh) ttileh = th;
    if (!(xformOpt & TJXOPT_NOOUTPUT)) {
      if (decomp(NULL, decompsrc ? &srcBuf : jpegBuf,
                 decompsrc ? &srcSize : jpegSize, NULL, tw, th, tsubsamp, 0,
                 fileName, ttilew, ttileh) == -1)
        goto bailout;
    } else if (quiet == 1) printf("N/A\n");

    for (i = 0; i < ntilesw * ntilesh; i++) {
      tjFree(jpegBuf[i]);  jpegBuf[i] = NULL;
    }
    free(jpegBuf);  jpegBuf = NULL;
    if (jpegSize) { free(jpegSize);  jpegSize = NULL; }

    if (tilew == w && tileh == h) break;
  }

bailout:
  if (file) { fclose(file);  file = NULL; }
  if (jpegBuf) {
    for (i = 0; i < ntilesw * ntilesh; i++) {
      if (jpegBuf[i]) tjFree(jpegBuf[i]);
      jpegBuf[i] = NULL;
    }
    free(jpegBuf);  jpegBuf = NULL;
  }
  if (jpegSize) { free(jpegSize);  jpegSize = NULL; }
  if (srcBuf) { free(srcBuf);  srcBuf = NULL; }
  if (t) { free(t);  t = NULL; }
  if (handle) { tjDestroy(handle);  handle = NULL; }
  return retval;
}