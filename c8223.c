DLLEXPORT int tjEncodeYUVPlanes(tjhandle handle, const unsigned char *srcBuf,
                                int width, int pitch, int height,
                                int pixelFormat, unsigned char **dstPlanes,
                                int *strides, int subsamp, int flags)
{
  JSAMPROW *row_pointer = NULL;
  JSAMPLE *_tmpbuf[MAX_COMPONENTS], *_tmpbuf2[MAX_COMPONENTS];
  JSAMPROW *tmpbuf[MAX_COMPONENTS], *tmpbuf2[MAX_COMPONENTS];
  JSAMPROW *outbuf[MAX_COMPONENTS];
  int i, retval = 0, row, pw0, ph0, pw[MAX_COMPONENTS], ph[MAX_COMPONENTS];
  JSAMPLE *ptr;
  jpeg_component_info *compptr;

  GET_CINSTANCE(handle);
  this->jerr.stopOnWarning = (flags & TJFLAG_STOPONWARNING) ? TRUE : FALSE;

  for (i = 0; i < MAX_COMPONENTS; i++) {
    tmpbuf[i] = NULL;  _tmpbuf[i] = NULL;
    tmpbuf2[i] = NULL;  _tmpbuf2[i] = NULL;  outbuf[i] = NULL;
  }

  if ((this->init & COMPRESS) == 0)
    THROW("tjEncodeYUVPlanes(): Instance has not been initialized for compression");

  if (srcBuf == NULL || width <= 0 || pitch < 0 || height <= 0 ||
      pixelFormat < 0 || pixelFormat >= TJ_NUMPF || !dstPlanes ||
      !dstPlanes[0] || subsamp < 0 || subsamp >= NUMSUBOPT)
    THROW("tjEncodeYUVPlanes(): Invalid argument");
  if (subsamp != TJSAMP_GRAY && (!dstPlanes[1] || !dstPlanes[2]))
    THROW("tjEncodeYUVPlanes(): Invalid argument");

  if (pixelFormat == TJPF_CMYK)
    THROW("tjEncodeYUVPlanes(): Cannot generate YUV images from CMYK pixels");

  if (pitch == 0) pitch = width * tjPixelSize[pixelFormat];

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

  if (setCompDefaults(cinfo, pixelFormat, subsamp, -1, flags) == -1) return -1;

  /* Execute only the parts of jpeg_start_compress() that we need.  If we
     were to call the whole jpeg_start_compress() function, then it would try
     to write the file headers, which could overflow the output buffer if the
     YUV image were very small. */
  if (cinfo->global_state != CSTATE_START)
    THROW("tjEncodeYUVPlanes(): libjpeg API is in the wrong state");
  (*cinfo->err->reset_error_mgr) ((j_common_ptr)cinfo);
  jinit_c_master_control(cinfo, FALSE);
  jinit_color_converter(cinfo);
  jinit_downsampler(cinfo);
  (*cinfo->cconvert->start_pass) (cinfo);

  pw0 = PAD(width, cinfo->max_h_samp_factor);
  ph0 = PAD(height, cinfo->max_v_samp_factor);

  if ((row_pointer = (JSAMPROW *)malloc(sizeof(JSAMPROW) * ph0)) == NULL)
    THROW("tjEncodeYUVPlanes(): Memory allocation failure");
  for (i = 0; i < height; i++) {
    if (flags & TJFLAG_BOTTOMUP)
      row_pointer[i] = (JSAMPROW)&srcBuf[(height - i - 1) * pitch];
    else
      row_pointer[i] = (JSAMPROW)&srcBuf[i * pitch];
  }
  if (height < ph0)
    for (i = height; i < ph0; i++) row_pointer[i] = row_pointer[height - 1];

  for (i = 0; i < cinfo->num_components; i++) {
    compptr = &cinfo->comp_info[i];
    _tmpbuf[i] = (JSAMPLE *)malloc(
      PAD((compptr->width_in_blocks * cinfo->max_h_samp_factor * DCTSIZE) /
          compptr->h_samp_factor, 32) *
      cinfo->max_v_samp_factor + 32);
    if (!_tmpbuf[i])
      THROW("tjEncodeYUVPlanes(): Memory allocation failure");
    tmpbuf[i] =
      (JSAMPROW *)malloc(sizeof(JSAMPROW) * cinfo->max_v_samp_factor);
    if (!tmpbuf[i])
      THROW("tjEncodeYUVPlanes(): Memory allocation failure");
    for (row = 0; row < cinfo->max_v_samp_factor; row++) {
      unsigned char *_tmpbuf_aligned =
        (unsigned char *)PAD((size_t)_tmpbuf[i], 32);

      tmpbuf[i][row] = &_tmpbuf_aligned[
        PAD((compptr->width_in_blocks * cinfo->max_h_samp_factor * DCTSIZE) /
            compptr->h_samp_factor, 32) * row];
    }
    _tmpbuf2[i] =
      (JSAMPLE *)malloc(PAD(compptr->width_in_blocks * DCTSIZE, 32) *
                        compptr->v_samp_factor + 32);
    if (!_tmpbuf2[i])
      THROW("tjEncodeYUVPlanes(): Memory allocation failure");
    tmpbuf2[i] = (JSAMPROW *)malloc(sizeof(JSAMPROW) * compptr->v_samp_factor);
    if (!tmpbuf2[i])
      THROW("tjEncodeYUVPlanes(): Memory allocation failure");
    for (row = 0; row < compptr->v_samp_factor; row++) {
      unsigned char *_tmpbuf2_aligned =
        (unsigned char *)PAD((size_t)_tmpbuf2[i], 32);

      tmpbuf2[i][row] =
        &_tmpbuf2_aligned[PAD(compptr->width_in_blocks * DCTSIZE, 32) * row];
    }
    pw[i] = pw0 * compptr->h_samp_factor / cinfo->max_h_samp_factor;
    ph[i] = ph0 * compptr->v_samp_factor / cinfo->max_v_samp_factor;
    outbuf[i] = (JSAMPROW *)malloc(sizeof(JSAMPROW) * ph[i]);
    if (!outbuf[i])
      THROW("tjEncodeYUVPlanes(): Memory allocation failure");
    ptr = dstPlanes[i];
    for (row = 0; row < ph[i]; row++) {
      outbuf[i][row] = ptr;
      ptr += (strides && strides[i] != 0) ? strides[i] : pw[i];
    }
  }

  if (setjmp(this->jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error. */
    retval = -1;  goto bailout;
  }

  for (row = 0; row < ph0; row += cinfo->max_v_samp_factor) {
    (*cinfo->cconvert->color_convert) (cinfo, &row_pointer[row], tmpbuf, 0,
                                       cinfo->max_v_samp_factor);
    (cinfo->downsample->downsample) (cinfo, tmpbuf, 0, tmpbuf2, 0);
    for (i = 0, compptr = cinfo->comp_info; i < cinfo->num_components;
         i++, compptr++)
      jcopy_sample_rows(tmpbuf2[i], 0, outbuf[i],
        row * compptr->v_samp_factor / cinfo->max_v_samp_factor,
        compptr->v_samp_factor, pw[i]);
  }
  cinfo->next_scanline += height;
  jpeg_abort_compress(cinfo);

bailout:
  if (cinfo->global_state > CSTATE_START) jpeg_abort_compress(cinfo);
  if (row_pointer) free(row_pointer);
  for (i = 0; i < MAX_COMPONENTS; i++) {
    if (tmpbuf[i] != NULL) free(tmpbuf[i]);
    if (_tmpbuf[i] != NULL) free(_tmpbuf[i]);
    if (tmpbuf2[i] != NULL) free(tmpbuf2[i]);
    if (_tmpbuf2[i] != NULL) free(_tmpbuf2[i]);
    if (outbuf[i] != NULL) free(outbuf[i]);
  }
  if (this->jerr.warning) retval = -1;
  this->jerr.stopOnWarning = FALSE;
  return retval;
}