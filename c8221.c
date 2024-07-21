DLLEXPORT int tjCompress2(tjhandle handle, const unsigned char *srcBuf,
                          int width, int pitch, int height, int pixelFormat,
                          unsigned char **jpegBuf, unsigned long *jpegSize,
                          int jpegSubsamp, int jpegQual, int flags)
{
  int i, retval = 0, alloc = 1;
  JSAMPROW *row_pointer = NULL;

  GET_CINSTANCE(handle)
  this->jerr.stopOnWarning = (flags & TJFLAG_STOPONWARNING) ? TRUE : FALSE;
  if ((this->init & COMPRESS) == 0)
    THROW("tjCompress2(): Instance has not been initialized for compression");

  if (srcBuf == NULL || width <= 0 || pitch < 0 || height <= 0 ||
      pixelFormat < 0 || pixelFormat >= TJ_NUMPF || jpegBuf == NULL ||
      jpegSize == NULL || jpegSubsamp < 0 || jpegSubsamp >= NUMSUBOPT ||
      jpegQual < 0 || jpegQual > 100)
    THROW("tjCompress2(): Invalid argument");

  if (pitch == 0) pitch = width * tjPixelSize[pixelFormat];

  if ((row_pointer = (JSAMPROW *)malloc(sizeof(JSAMPROW) * height)) == NULL)
    THROW("tjCompress2(): Memory allocation failure");

  if (setjmp(this->jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error. */
    retval = -1;  goto bailout;
  }

  cinfo->image_width = width;
  cinfo->image_height = height;

#ifndef NO_PUTENV
  if (flags & TJFLAG_FORCEMMX) putenv("JSIMD_FORCEMMX=1");
  else if (flags & TJFLAG_FORCESSE) putenv("JSIMD_FORCESSE=1");
  else if (flags & TJFLAG_FORCESSE2) putenv("JSIMD_FORCESSE2=1");
#endif

  if (flags & TJFLAG_NOREALLOC) {
    alloc = 0;  *jpegSize = tjBufSize(width, height, jpegSubsamp);
  }
  jpeg_mem_dest_tj(cinfo, jpegBuf, jpegSize, alloc);
  if (setCompDefaults(cinfo, pixelFormat, jpegSubsamp, jpegQual, flags) == -1)
    return -1;

  jpeg_start_compress(cinfo, TRUE);
  for (i = 0; i < height; i++) {
    if (flags & TJFLAG_BOTTOMUP)
      row_pointer[i] = (JSAMPROW)&srcBuf[(height - i - 1) * pitch];
    else
      row_pointer[i] = (JSAMPROW)&srcBuf[i * pitch];
  }
  while (cinfo->next_scanline < cinfo->image_height)
    jpeg_write_scanlines(cinfo, &row_pointer[cinfo->next_scanline],
                         cinfo->image_height - cinfo->next_scanline);
  jpeg_finish_compress(cinfo);

bailout:
  if (cinfo->global_state > CSTATE_START) jpeg_abort_compress(cinfo);
  if (row_pointer) free(row_pointer);
  if (this->jerr.warning) retval = -1;
  this->jerr.stopOnWarning = FALSE;
  return retval;
}