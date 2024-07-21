DLLEXPORT int tjDecompress2(tjhandle handle, const unsigned char *jpegBuf,
                            unsigned long jpegSize, unsigned char *dstBuf,
                            int width, int pitch, int height, int pixelFormat,
                            int flags)
{
  JSAMPROW *row_pointer = NULL;
  int i, retval = 0, jpegwidth, jpegheight, scaledw, scaledh;

  GET_DINSTANCE(handle);
  this->jerr.stopOnWarning = (flags & TJFLAG_STOPONWARNING) ? TRUE : FALSE;
  if ((this->init & DECOMPRESS) == 0)
    THROW("tjDecompress2(): Instance has not been initialized for decompression");

  if (jpegBuf == NULL || jpegSize <= 0 || dstBuf == NULL || width < 0 ||
      pitch < 0 || height < 0 || pixelFormat < 0 || pixelFormat >= TJ_NUMPF)
    THROW("tjDecompress2(): Invalid argument");

#ifndef NO_PUTENV
  if (flags & TJFLAG_FORCEMMX) putenv("JSIMD_FORCEMMX=1");
  else if (flags & TJFLAG_FORCESSE) putenv("JSIMD_FORCESSE=1");
  else if (flags & TJFLAG_FORCESSE2) putenv("JSIMD_FORCESSE2=1");
#endif

  if (setjmp(this->jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error. */
    retval = -1;  goto bailout;
  }

  jpeg_mem_src_tj(dinfo, jpegBuf, jpegSize);
  jpeg_read_header(dinfo, TRUE);
  this->dinfo.out_color_space = pf2cs[pixelFormat];
  if (flags & TJFLAG_FASTDCT) this->dinfo.dct_method = JDCT_FASTEST;
  if (flags & TJFLAG_FASTUPSAMPLE) dinfo->do_fancy_upsampling = FALSE;

  jpegwidth = dinfo->image_width;  jpegheight = dinfo->image_height;
  if (width == 0) width = jpegwidth;
  if (height == 0) height = jpegheight;
  for (i = 0; i < NUMSF; i++) {
    scaledw = TJSCALED(jpegwidth, sf[i]);
    scaledh = TJSCALED(jpegheight, sf[i]);
    if (scaledw <= width && scaledh <= height)
      break;
  }
  if (i >= NUMSF)
    THROW("tjDecompress2(): Could not scale down to desired image dimensions");
  width = scaledw;  height = scaledh;
  dinfo->scale_num = sf[i].num;
  dinfo->scale_denom = sf[i].denom;

  jpeg_start_decompress(dinfo);
  if (pitch == 0) pitch = dinfo->output_width * tjPixelSize[pixelFormat];

  if ((row_pointer =
       (JSAMPROW *)malloc(sizeof(JSAMPROW) * dinfo->output_height)) == NULL)
    THROW("tjDecompress2(): Memory allocation failure");
  if (setjmp(this->jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error. */
    retval = -1;  goto bailout;
  }
  for (i = 0; i < (int)dinfo->output_height; i++) {
    if (flags & TJFLAG_BOTTOMUP)
      row_pointer[i] = &dstBuf[(dinfo->output_height - i - 1) * pitch];
    else
      row_pointer[i] = &dstBuf[i * pitch];
  }
  while (dinfo->output_scanline < dinfo->output_height)
    jpeg_read_scanlines(dinfo, &row_pointer[dinfo->output_scanline],
                        dinfo->output_height - dinfo->output_scanline);
  jpeg_finish_decompress(dinfo);

bailout:
  if (dinfo->global_state > DSTATE_START) jpeg_abort_decompress(dinfo);
  if (row_pointer) free(row_pointer);
  if (this->jerr.warning) retval = -1;
  this->jerr.stopOnWarning = FALSE;
  return retval;
}