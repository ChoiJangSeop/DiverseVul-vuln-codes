static int fullTest(unsigned char *srcBuf, int w, int h, int subsamp,
                    int jpegQual, char *fileName)
{
  char tempStr[1024], tempStr2[80];
  FILE *file = NULL;
  tjhandle handle = NULL;
  unsigned char **jpegBuf = NULL, *yuvBuf = NULL, *tmpBuf = NULL, *srcPtr,
    *srcPtr2;
  double start, elapsed, elapsedEncode;
  int totalJpegSize = 0, row, col, i, tilew = w, tileh = h, retval = 0;
  int iter, yuvSize = 0;
  unsigned long *jpegSize = NULL;
  int ps = tjPixelSize[pf];
  int ntilesw = 1, ntilesh = 1, pitch = w * ps;
  const char *pfStr = pixFormatStr[pf];

  if ((tmpBuf = (unsigned char *)malloc(pitch * h)) == NULL)
    THROW_UNIX("allocating temporary image buffer");

  if (!quiet)
    printf(">>>>>  %s (%s) <--> JPEG %s Q%d  <<<<<\n", pfStr,
           (flags & TJFLAG_BOTTOMUP) ? "Bottom-up" : "Top-down",
           subNameLong[subsamp], jpegQual);

  for (tilew = doTile ? 8 : w, tileh = doTile ? 8 : h; ;
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

    if ((flags & TJFLAG_NOREALLOC) != 0)
      for (i = 0; i < ntilesw * ntilesh; i++) {
        if ((jpegBuf[i] = (unsigned char *)
                          tjAlloc(tjBufSize(tilew, tileh, subsamp))) == NULL)
          THROW_UNIX("allocating JPEG tiles");
      }

    /* Compression test */
    if (quiet == 1)
      printf("%-4s (%s)  %-5s    %-3d   ", pfStr,
             (flags & TJFLAG_BOTTOMUP) ? "BU" : "TD", subNameLong[subsamp],
             jpegQual);
    for (i = 0; i < h; i++)
      memcpy(&tmpBuf[pitch * i], &srcBuf[w * ps * i], w * ps);
    if ((handle = tjInitCompress()) == NULL)
      THROW_TJ("executing tjInitCompress()");

    if (doYUV) {
      yuvSize = tjBufSizeYUV2(tilew, yuvPad, tileh, subsamp);
      if ((yuvBuf = (unsigned char *)malloc(yuvSize)) == NULL)
        THROW_UNIX("allocating YUV buffer");
      memset(yuvBuf, 127, yuvSize);
    }

    /* Benchmark */
    iter = -1;
    elapsed = elapsedEncode = 0.;
    while (1) {
      int tile = 0;

      totalJpegSize = 0;
      start = getTime();
      for (row = 0, srcPtr = srcBuf; row < ntilesh;
           row++, srcPtr += pitch * tileh) {
        for (col = 0, srcPtr2 = srcPtr; col < ntilesw;
             col++, tile++, srcPtr2 += ps * tilew) {
          int width = min(tilew, w - col * tilew);
          int height = min(tileh, h - row * tileh);

          if (doYUV) {
            double startEncode = getTime();

            if (tjEncodeYUV3(handle, srcPtr2, width, pitch, height, pf, yuvBuf,
                             yuvPad, subsamp, flags) == -1)
              THROW_TJ("executing tjEncodeYUV3()");
            if (iter >= 0) elapsedEncode += getTime() - startEncode;
            if (tjCompressFromYUV(handle, yuvBuf, width, yuvPad, height,
                                  subsamp, &jpegBuf[tile], &jpegSize[tile],
                                  jpegQual, flags) == -1)
              THROW_TJ("executing tjCompressFromYUV()");
          } else {
            if (tjCompress2(handle, srcPtr2, width, pitch, height, pf,
                            &jpegBuf[tile], &jpegSize[tile], subsamp, jpegQual,
                            flags) == -1)
              THROW_TJ("executing tjCompress2()");
          }
          totalJpegSize += jpegSize[tile];
        }
      }
      elapsed += getTime() - start;
      if (iter >= 0) {
        iter++;
        if (elapsed >= benchTime) break;
      } else if (elapsed >= warmup) {
        iter = 0;
        elapsed = elapsedEncode = 0.;
      }
    }
    if (doYUV) elapsed -= elapsedEncode;

    if (tjDestroy(handle) == -1) THROW_TJ("executing tjDestroy()");
    handle = NULL;

    if (quiet == 1) printf("%-5d  %-5d   ", tilew, tileh);
    if (quiet) {
      if (doYUV)
        printf("%-6s%s",
               sigfig((double)(w * h) / 1000000. *
                      (double)iter / elapsedEncode, 4, tempStr, 1024),
               quiet == 2 ? "\n" : "  ");
      printf("%-6s%s",
             sigfig((double)(w * h) / 1000000. * (double)iter / elapsed, 4,
                    tempStr, 1024),
             quiet == 2 ? "\n" : "  ");
      printf("%-6s%s",
             sigfig((double)(w * h * ps) / (double)totalJpegSize, 4, tempStr2,
                    80),
             quiet == 2 ? "\n" : "  ");
    } else {
      printf("\n%s size: %d x %d\n", doTile ? "Tile" : "Image", tilew, tileh);
      if (doYUV) {
        printf("Encode YUV    --> Frame rate:         %f fps\n",
               (double)iter / elapsedEncode);
        printf("                  Output image size:  %d bytes\n", yuvSize);
        printf("                  Compression ratio:  %f:1\n",
               (double)(w * h * ps) / (double)yuvSize);
        printf("                  Throughput:         %f Megapixels/sec\n",
               (double)(w * h) / 1000000. * (double)iter / elapsedEncode);
        printf("                  Output bit stream:  %f Megabits/sec\n",
               (double)yuvSize * 8. / 1000000. * (double)iter / elapsedEncode);
      }
      printf("%s --> Frame rate:         %f fps\n",
             doYUV ? "Comp from YUV" : "Compress     ",
             (double)iter / elapsed);
      printf("                  Output image size:  %d bytes\n",
             totalJpegSize);
      printf("                  Compression ratio:  %f:1\n",
             (double)(w * h * ps) / (double)totalJpegSize);
      printf("                  Throughput:         %f Megapixels/sec\n",
             (double)(w * h) / 1000000. * (double)iter / elapsed);
      printf("                  Output bit stream:  %f Megabits/sec\n",
             (double)totalJpegSize * 8. / 1000000. * (double)iter / elapsed);
    }
    if (tilew == w && tileh == h && doWrite) {
      snprintf(tempStr, 1024, "%s_%s_Q%d.jpg", fileName, subName[subsamp],
               jpegQual);
      if ((file = fopen(tempStr, "wb")) == NULL)
        THROW_UNIX("opening reference image");
      if (fwrite(jpegBuf[0], jpegSize[0], 1, file) != 1)
        THROW_UNIX("writing reference image");
      fclose(file);  file = NULL;
      if (!quiet) printf("Reference image written to %s\n", tempStr);
    }

    /* Decompression test */
    if (!compOnly) {
      if (decomp(srcBuf, jpegBuf, jpegSize, tmpBuf, w, h, subsamp, jpegQual,
                 fileName, tilew, tileh) == -1)
        goto bailout;
    }

    for (i = 0; i < ntilesw * ntilesh; i++) {
      if (jpegBuf[i]) tjFree(jpegBuf[i]);
      jpegBuf[i] = NULL;
    }
    free(jpegBuf);  jpegBuf = NULL;
    free(jpegSize);  jpegSize = NULL;
    if (doYUV) {
      free(yuvBuf);  yuvBuf = NULL;
    }

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
  if (yuvBuf) { free(yuvBuf);  yuvBuf = NULL; }
  if (jpegSize) { free(jpegSize);  jpegSize = NULL; }
  if (tmpBuf) { free(tmpBuf);  tmpBuf = NULL; }
  if (handle) { tjDestroy(handle);  handle = NULL; }
  return retval;
}