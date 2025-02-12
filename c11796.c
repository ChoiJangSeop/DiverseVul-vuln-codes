h2v1_merged_upsample_565D_internal(j_decompress_ptr cinfo,
                                   JSAMPIMAGE input_buf,
                                   JDIMENSION in_row_group_ctr,
                                   JSAMPARRAY output_buf)
{
  my_upsample_ptr upsample = (my_upsample_ptr)cinfo->upsample;
  register int y, cred, cgreen, cblue;
  int cb, cr;
  register JSAMPROW outptr;
  JSAMPROW inptr0, inptr1, inptr2;
  JDIMENSION col;
  /* copy these pointers into registers if possible */
  register JSAMPLE *range_limit = cinfo->sample_range_limit;
  int *Crrtab = upsample->Cr_r_tab;
  int *Cbbtab = upsample->Cb_b_tab;
  JLONG *Crgtab = upsample->Cr_g_tab;
  JLONG *Cbgtab = upsample->Cb_g_tab;
  JLONG d0 = dither_matrix[cinfo->output_scanline & DITHER_MASK];
  unsigned int r, g, b;
  JLONG rgb;
  SHIFT_TEMPS

  inptr0 = input_buf[0][in_row_group_ctr];
  inptr1 = input_buf[1][in_row_group_ctr];
  inptr2 = input_buf[2][in_row_group_ctr];
  outptr = output_buf[0];

  /* Loop for each pair of output pixels */
  for (col = cinfo->output_width >> 1; col > 0; col--) {
    /* Do the chroma part of the calculation */
    cb = GETJSAMPLE(*inptr1++);
    cr = GETJSAMPLE(*inptr2++);
    cred = Crrtab[cr];
    cgreen = (int)RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr], SCALEBITS);
    cblue = Cbbtab[cb];

    /* Fetch 2 Y values and emit 2 pixels */
    y  = GETJSAMPLE(*inptr0++);
    r = range_limit[DITHER_565_R(y + cred, d0)];
    g = range_limit[DITHER_565_G(y + cgreen, d0)];
    b = range_limit[DITHER_565_B(y + cblue, d0)];
    d0 = DITHER_ROTATE(d0);
    rgb = PACK_SHORT_565(r, g, b);

    y  = GETJSAMPLE(*inptr0++);
    r = range_limit[DITHER_565_R(y + cred, d0)];
    g = range_limit[DITHER_565_G(y + cgreen, d0)];
    b = range_limit[DITHER_565_B(y + cblue, d0)];
    d0 = DITHER_ROTATE(d0);
    rgb = PACK_TWO_PIXELS(rgb, PACK_SHORT_565(r, g, b));

    WRITE_TWO_PIXELS(outptr, rgb);
    outptr += 4;
  }

  /* If image width is odd, do the last output column separately */
  if (cinfo->output_width & 1) {
    cb = GETJSAMPLE(*inptr1);
    cr = GETJSAMPLE(*inptr2);
    cred = Crrtab[cr];
    cgreen = (int)RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr], SCALEBITS);
    cblue = Cbbtab[cb];
    y  = GETJSAMPLE(*inptr0);
    r = range_limit[DITHER_565_R(y + cred, d0)];
    g = range_limit[DITHER_565_G(y + cgreen, d0)];
    b = range_limit[DITHER_565_B(y + cblue, d0)];
    rgb = PACK_SHORT_565(r, g, b);
    *(INT16 *)outptr = (INT16)rgb;
  }
}