DLLEXPORT int tjDecodeYUVPlanes(tjhandle handle,
                                const unsigned char **srcPlanes,
                                const int *strides, int subsamp,
                                unsigned char *dstBuf, int width, int pitch,
                                int height, int pixelFormat, int flags)
{
  JSAMPROW *row_pointer = NULL;
  JSAMPLE *_tmpbuf[MAX_COMPONENTS];
  JSAMPROW *tmpbuf[MAX_COMPONENTS], *inbuf[MAX_COMPONENTS];
  int i, retval = 0, row, pw0, ph0, pw[MAX_COMPONENTS], ph[MAX_COMPONENTS];
  JSAMPLE *ptr;
  jpeg_component_info *compptr;
  int (*old_read_markers) (j_decompress_ptr);
  void (*old_reset_marker_reader) (j_decompress_ptr);

  GET_DINSTANCE(handle);
  this->jerr.stopOnWarning = (flags & TJFLAG_STOPONWARNING) ? TRUE : FALSE;

  for (i = 0; i < MAX_COMPONENTS; i++) {
    tmpbuf[i] = NULL;  _tmpbuf[i] = NULL;  inbuf[i] = NULL;
  }

  if ((this->init & DECOMPRESS) == 0)
    THROW("tjDecodeYUVPlanes(): Instance has not been initialized for decompression");

  if (!srcPlanes || !srcPlanes[0] || subsamp < 0 || subsamp >= NUMSUBOPT ||
      dstBuf == NULL || width <= 0 || pitch < 0 || height <= 0 ||
      pixelFormat < 0 || pixelFormat >= TJ_NUMPF)
    THROW("tjDecodeYUVPlanes(): Invalid argument");
  if (subsamp != TJSAMP_GRAY && (!srcPlanes[1] || !srcPlanes[2]))
    THROW("tjDecodeYUVPlanes(): Invalid argument");

  if (setjmp(this->jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error. */
    retval = -1;  goto bailout;
  }

  if (pixelFormat == TJPF_CMYK)
    THROW("tjDecodeYUVPlanes(): Cannot decode YUV images into CMYK pixels.");

  if (pitch == 0) pitch = width * tjPixelSize[pixelFormat];
  dinfo->image_width = width;
  dinfo->image_height = height;

#ifndef NO_PUTENV
  if (flags & TJFLAG_FORCEMMX) putenv("JSIMD_FORCEMMX=1");
  else if (flags & TJFLAG_FORCESSE) putenv("JSIMD_FORCESSE=1");
  else if (flags & TJFLAG_FORCESSE2) putenv("JSIMD_FORCESSE2=1");
#endif

  if (setDecodeDefaults(dinfo, pixelFormat, subsamp, flags) == -1) {
    retval = -1;  goto bailout;
  }
  old_read_markers = dinfo->marker->read_markers;
  dinfo->marker->read_markers = my_read_markers;
  old_reset_marker_reader = dinfo->marker->reset_marker_reader;
  dinfo->marker->reset_marker_reader = my_reset_marker_reader;
  jpeg_read_header(dinfo, TRUE);
  dinfo->marker->read_markers = old_read_markers;
  dinfo->marker->reset_marker_reader = old_reset_marker_reader;

  this->dinfo.out_color_space = pf2cs[pixelFormat];
  if (flags & TJFLAG_FASTDCT) this->dinfo.dct_method = JDCT_FASTEST;
  dinfo->do_fancy_upsampling = FALSE;
  dinfo->Se = DCTSIZE2 - 1;
  jinit_master_decompress(dinfo);
  (*dinfo->upsample->start_pass) (dinfo);

  pw0 = PAD(width, dinfo->max_h_samp_factor);
  ph0 = PAD(height, dinfo->max_v_samp_factor);

  if (pitch == 0) pitch = dinfo->output_width * tjPixelSize[pixelFormat];

  if ((row_pointer = (JSAMPROW *)malloc(sizeof(JSAMPROW) * ph0)) == NULL)
    THROW("tjDecodeYUVPlanes(): Memory allocation failure");
  for (i = 0; i < height; i++) {
    if (flags & TJFLAG_BOTTOMUP)
      row_pointer[i] = &dstBuf[(height - i - 1) * pitch];
    else
      row_pointer[i] = &dstBuf[i * pitch];
  }
  if (height < ph0)
    for (i = height; i < ph0; i++) row_pointer[i] = row_pointer[height - 1];

  for (i = 0; i < dinfo->num_components; i++) {
    compptr = &dinfo->comp_info[i];
    _tmpbuf[i] =
      (JSAMPLE *)malloc(PAD(compptr->width_in_blocks * DCTSIZE, 32) *
                        compptr->v_samp_factor + 32);
    if (!_tmpbuf[i])
      THROW("tjDecodeYUVPlanes(): Memory allocation failure");
    tmpbuf[i] = (JSAMPROW *)malloc(sizeof(JSAMPROW) * compptr->v_samp_factor);
    if (!tmpbuf[i])
      THROW("tjDecodeYUVPlanes(): Memory allocation failure");
    for (row = 0; row < compptr->v_samp_factor; row++) {
      unsigned char *_tmpbuf_aligned =
        (unsigned char *)PAD((size_t)_tmpbuf[i], 32);

      tmpbuf[i][row] =
        &_tmpbuf_aligned[PAD(compptr->width_in_blocks * DCTSIZE, 32) * row];
    }
    pw[i] = pw0 * compptr->h_samp_factor / dinfo->max_h_samp_factor;
    ph[i] = ph0 * compptr->v_samp_factor / dinfo->max_v_samp_factor;
    inbuf[i] = (JSAMPROW *)malloc(sizeof(JSAMPROW) * ph[i]);
    if (!inbuf[i])
      THROW("tjDecodeYUVPlanes(): Memory allocation failure");
    ptr = (JSAMPLE *)srcPlanes[i];
    for (row = 0; row < ph[i]; row++) {
      inbuf[i][row] = ptr;
      ptr += (strides && strides[i] != 0) ? strides[i] : pw[i];
    }
  }

  if (setjmp(this->jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error. */
    retval = -1;  goto bailout;
  }

  for (row = 0; row < ph0; row += dinfo->max_v_samp_factor) {
    JDIMENSION inrow = 0, outrow = 0;

    for (i = 0, compptr = dinfo->comp_info; i < dinfo->num_components;
         i++, compptr++)
      jcopy_sample_rows(inbuf[i],
        row * compptr->v_samp_factor / dinfo->max_v_samp_factor, tmpbuf[i], 0,
        compptr->v_samp_factor, pw[i]);
    (dinfo->upsample->upsample) (dinfo, tmpbuf, &inrow,
                                 dinfo->max_v_samp_factor, &row_pointer[row],
                                 &outrow, dinfo->max_v_samp_factor);
  }
  jpeg_abort_decompress(dinfo);

bailout:
  if (dinfo->global_state > DSTATE_START) jpeg_abort_decompress(dinfo);
  if (row_pointer) free(row_pointer);
  for (i = 0; i < MAX_COMPONENTS; i++) {
    if (tmpbuf[i] != NULL) free(tmpbuf[i]);
    if (_tmpbuf[i] != NULL) free(_tmpbuf[i]);
    if (inbuf[i] != NULL) free(inbuf[i]);
  }
  if (this->jerr.warning) retval = -1;
  this->jerr.stopOnWarning = FALSE;
  return retval;
}