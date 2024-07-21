start_input_ppm (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
  ppm_source_ptr source = (ppm_source_ptr) sinfo;
  int c;
  unsigned int w, h, maxval;
  boolean need_iobuffer, use_raw_buffer, need_rescale;

  if (getc(source->pub.input_file) != 'P')
    ERREXIT(cinfo, JERR_PPM_NOT);

  c = getc(source->pub.input_file); /* subformat discriminator character */

  /* detect unsupported variants (ie, PBM) before trying to read header */
  switch (c) {
  case '2':                     /* it's a text-format PGM file */
  case '3':                     /* it's a text-format PPM file */
  case '5':                     /* it's a raw-format PGM file */
  case '6':                     /* it's a raw-format PPM file */
    break;
  default:
    ERREXIT(cinfo, JERR_PPM_NOT);
    break;
  }

  /* fetch the remaining header info */
  w = read_pbm_integer(cinfo, source->pub.input_file);
  h = read_pbm_integer(cinfo, source->pub.input_file);
  maxval = read_pbm_integer(cinfo, source->pub.input_file);

  if (w <= 0 || h <= 0 || maxval <= 0) /* error check */
    ERREXIT(cinfo, JERR_PPM_NOT);

  cinfo->data_precision = BITS_IN_JSAMPLE; /* we always rescale data to this */
  cinfo->image_width = (JDIMENSION) w;
  cinfo->image_height = (JDIMENSION) h;

  /* initialize flags to most common settings */
  need_iobuffer = TRUE;         /* do we need an I/O buffer? */
  use_raw_buffer = FALSE;       /* do we map input buffer onto I/O buffer? */
  need_rescale = TRUE;          /* do we need a rescale array? */

  switch (c) {
  case '2':                     /* it's a text-format PGM file */
    cinfo->input_components = 1;
    cinfo->in_color_space = JCS_GRAYSCALE;
    TRACEMS2(cinfo, 1, JTRC_PGM_TEXT, w, h);
    source->pub.get_pixel_rows = get_text_gray_row;
    need_iobuffer = FALSE;
    break;

  case '3':                     /* it's a text-format PPM file */
    cinfo->input_components = 3;
    cinfo->in_color_space = JCS_RGB;
    TRACEMS2(cinfo, 1, JTRC_PPM_TEXT, w, h);
    source->pub.get_pixel_rows = get_text_rgb_row;
    need_iobuffer = FALSE;
    break;

  case '5':                     /* it's a raw-format PGM file */
    cinfo->input_components = 1;
    cinfo->in_color_space = JCS_GRAYSCALE;
    TRACEMS2(cinfo, 1, JTRC_PGM, w, h);
    if (maxval > 255) {
      source->pub.get_pixel_rows = get_word_gray_row;
    } else if (maxval == MAXJSAMPLE && sizeof(JSAMPLE) == sizeof(U_CHAR)) {
      source->pub.get_pixel_rows = get_raw_row;
      use_raw_buffer = TRUE;
      need_rescale = FALSE;
    } else {
      source->pub.get_pixel_rows = get_scaled_gray_row;
    }
    break;

  case '6':                     /* it's a raw-format PPM file */
    cinfo->input_components = 3;
    cinfo->in_color_space = JCS_RGB;
    TRACEMS2(cinfo, 1, JTRC_PPM, w, h);
    if (maxval > 255) {
      source->pub.get_pixel_rows = get_word_rgb_row;
    } else if (maxval == MAXJSAMPLE && sizeof(JSAMPLE) == sizeof(U_CHAR)) {
      source->pub.get_pixel_rows = get_raw_row;
      use_raw_buffer = TRUE;
      need_rescale = FALSE;
    } else {
      source->pub.get_pixel_rows = get_scaled_rgb_row;
    }
    break;
  }

  /* Allocate space for I/O buffer: 1 or 3 bytes or words/pixel. */
  if (need_iobuffer) {
    source->buffer_width = (size_t) w * cinfo->input_components *
      ((maxval<=255) ? sizeof(U_CHAR) : (2*sizeof(U_CHAR)));
    source->iobuffer = (U_CHAR *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                  source->buffer_width);
  }

  /* Create compressor input buffer. */
  if (use_raw_buffer) {
    /* For unscaled raw-input case, we can just map it onto the I/O buffer. */
    /* Synthesize a JSAMPARRAY pointer structure */
    source->pixrow = (JSAMPROW) source->iobuffer;
    source->pub.buffer = & source->pixrow;
    source->pub.buffer_height = 1;
  } else {
    /* Need to translate anyway, so make a separate sample buffer. */
    source->pub.buffer = (*cinfo->mem->alloc_sarray)
      ((j_common_ptr) cinfo, JPOOL_IMAGE,
       (JDIMENSION) w * cinfo->input_components, (JDIMENSION) 1);
    source->pub.buffer_height = 1;
  }

  /* Compute the rescaling array if required. */
  if (need_rescale) {
    INT32 val, half_maxval;

    /* On 16-bit-int machines we have to be careful of maxval = 65535 */
    source->rescale = (JSAMPLE *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                  (size_t) (((long) maxval + 1L) * sizeof(JSAMPLE)));
    half_maxval = maxval / 2;
    for (val = 0; val <= (INT32) maxval; val++) {
      /* The multiplication here must be done in 32 bits to avoid overflow */
      source->rescale[val] = (JSAMPLE) ((val*MAXJSAMPLE + half_maxval)/maxval);
    }
  }
}